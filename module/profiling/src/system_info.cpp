#include "oops/system_info.h"

#include <array>
#include <cassert>
#include <cctype>
#include <charconv>
#include <cstdio>
#include <fstream>
#include <functional>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string_view>
#include <variant>

#include <sys/sysmacros.h>

#include "fmt/format.h"
#include "scn/scan.h"

#include "oops/format.h"
#include "oops/str.h"
#include "oops/type_list.h"

namespace oops {
using namespace meta;
// 获取命令行输出
std::string GetCmdOutput(const std::string &cmd) {
    std::unique_ptr<FILE, int (*)(FILE *)> p{popen(cmd.c_str(), "r"), &pclose};
    std::string output;
    if (!p) {
        return output;
    }

    char buffer[4096];
    while (fgets(buffer, sizeof(buffer), p.get()) != nullptr) {
        output += buffer;
    }
    return output;
}

// 解析和格式化字段的特化注册
template <typename T>
bool ParseField(std::string_view s, T &t, std::string_view ctx = "{}") {
    auto scan_res{scn::scan<T>(s, ctx)};
    if (scan_res) {
        t = scan_res->value();
    }
    return bool{scan_res};
}

template <typename T>
std::string FormatField(const T &t, std::string_view ctx = "{}") {
    return fmt::format(ctx, t);
}

// Storage特化
template <typename R, typename P>
bool ParseField(std::string_view s, Storage<R, P> &storage, std::string_view ctx = "{}") {
    R r{};
    auto res{ParseField<R>(s, r, ctx)};
    storage = Storage<R, P>{r};
    return res;
}

template <typename R, typename P>
std::string FormatField(const Storage<R, P> &storage, std::string_view ctx = "{}") {
    return FormatField(storage.Count(), ctx);
}

// proc::smaps::VmaExt::VmFlags特化
struct VmFlagsEntry {
    std::string_view key;
    void (*set)(proc::smaps::VmaExt::VmFlags &);
    bool (*get)(const proc::smaps::VmaExt::VmFlags &);
};

#define ENTRY(flag)                                                      \
    {                                                                    \
        #flag, [](proc::smaps::VmaExt::VmFlags &f) { f.flag = true; },   \
            [](const proc::smaps::VmaExt::VmFlags &f) { return f.flag; } \
    }
constexpr VmFlagsEntry VM_FLAGS_TABLE[]{ENTRY(rd), ENTRY(wr), ENTRY(ex), ENTRY(sh), ENTRY(mr), ENTRY(mw), ENTRY(me),
                                        ENTRY(ms), ENTRY(gd), ENTRY(pf), ENTRY(dw), ENTRY(lo), ENTRY(io), ENTRY(sr),
                                        ENTRY(rr), ENTRY(dc), ENTRY(de), ENTRY(ac), ENTRY(nr), ENTRY(ht), ENTRY(sf),
                                        ENTRY(nl), ENTRY(ar), ENTRY(wf), ENTRY(dd), ENTRY(sd), ENTRY(mm), ENTRY(hg),
                                        ENTRY(nh), ENTRY(mg), ENTRY(um), ENTRY(uw)};
#undef ENTRY

template <>
bool ParseField(std::string_view s, proc::smaps::VmaExt::VmFlags &vm_flags, std::string_view) {
    bool failed{false};
    for (auto token : Split<std::vector<std::string_view>>(s)) {
        auto it{std::find_if(
            std::begin(VM_FLAGS_TABLE), std::end(VM_FLAGS_TABLE), [token](auto &entry) { return entry.key == token; })};
        if (it != std::end(VM_FLAGS_TABLE)) {
            it->set(vm_flags);
        } else {
            failed = true;
        }
    }
    return !failed;
}

template <>
std::string FormatField(const proc::smaps::VmaExt::VmFlags &vm_flags, std::string_view) {
    std::string s;
    s.reserve(64);
    for (const auto &item : VM_FLAGS_TABLE) {
        if (item.get(vm_flags)) {
            s += item.key;
            s += ' ';
        }
    }
    if (!s.empty()) {
        s.pop_back();
    }
    return s;
}

template <typename S, typename F, typename TL>
class KeyValueParser {
    template <typename T>
    using MemberPtr = T S::*; // 辅助元函数生成T S::*成员指针
    using MemberPtrList = oops::meta::TransformT<MemberPtr, TL>;

public:
    static_assert(std::is_enum_v<F>);

    using Struct = S;
    using Field = F;
    using MemberPtrVar = oops::meta::ApplyT<std::variant, MemberPtrList>;

    struct Entry {
        Field field;
        std::string key;
        MemberPtrVar member_ptr_var;
        std::string suffix{};
    };

    explicit KeyValueParser(std::initializer_list<Entry> entries) : field_table_{entries} {}
    KeyValueParser(std::initializer_list<Entry> entries, std::string delim)
        : field_table_{entries}, delim_{std::move(delim)} {}
    KeyValueParser(std::initializer_list<Entry> entries, bool (*stop)(std::string_view))
        : field_table_{entries}, stop_{stop} {}
    KeyValueParser(std::initializer_list<Entry> entries, std::string delim, bool (*stop)(std::string_view))
        : field_table_{entries}, delim_{std::move(delim)}, stop_{stop} {}

    static bool ParseMemberPtrVar(std::string_view s, MemberPtrVar member_ptr_var, Struct &obj) {
        auto f = [s, &obj](auto member_ptr) {
            auto &member{obj.*member_ptr};
            return ParseField(s, member, "{}");
        };
        return std::visit(f, member_ptr_var);
    }

    static std::string FormatMemberPtrVar(MemberPtrVar member_ptr_var, const Struct &obj) {
        auto f = [&obj](auto member_ptr) -> std::string {
            auto &member{obj.*member_ptr};
            return FormatField(member, "{}");
        };
        return std::visit(f, member_ptr_var);
    }

    // 根据注册表向object中各字段填值，更适合引用输出
    EnumBitset<Field> Parse(std::istream &is, Struct &object, const EnumBitset<Field> &field_mask) {
        EnumBitset<Field> parsed;
        std::streampos checkpoint{is.tellg()};
        std::string line_buf;
        while (std::getline(is, line_buf)) {
            std::string_view line{line_buf};
            if (stop_ && stop_(line)) {
                is.seekg(checkpoint);
                break;
            }
            checkpoint = is.tellg();
            std::size_t pos{line.find(delim_)};
            if (pos == line.npos) {
                continue;
            }

            std::string_view key{Strip(line.substr(0, pos))};
            if (key.empty()) {
                continue;
            }

            auto it{std::find_if(
                field_table_.begin(), field_table_.end(), [&key](const auto &entry) { return entry.key == key; })};
            if (it == field_table_.end()) {
                continue; // 未找到匹配的entry
            }
            if (!field_mask.Test(it->field)) {
                continue; // 不用解析
            }
            if (parsed.Test(it->field)) {
                continue; // 已解析过
            }

            std::string_view value{Strip(line.substr(pos + 1))};
            if (value.empty()) {
                continue;
            }

            ParseMemberPtrVar(value, it->member_ptr_var, object);
            parsed |= it->field;
            if (parsed == field_mask) {
                break; // 已解析全量
            }
        }
        if (stop_) {
            while (std::getline(is, line_buf)) {
                if (stop_(line_buf)) {
                    is.seekg(checkpoint);
                    break;
                }
                checkpoint = is.tellg();
            }
        }
        return parsed;
    }

    void Format(std::ostream &os, const Struct &object, const EnumBitset<Field> &field_mask = ~EnumBitset<Field>{}) {
        FTable ftable;
        for (const auto &entry : field_table_) {
            if (field_mask.Test(entry.field)) {
                std::string value{FormatMemberPtrVar(entry.member_ptr_var, object)};
                if (!value.empty()) {
                    ftable.AppendRow(std::string{entry.key} + delim_, value, entry.suffix);
                }
            }
        }
        os << ftable;
    }

private:
    std::vector<Entry> field_table_;
    std::string delim_{":"};
    bool (*stop_)(std::string_view){nullptr};
};

namespace proc {
namespace status {
KeyValueParser<Info, Field, TypeList<std::size_t>> kvparser{
    {{Field::VM_PEAK, "VmPeak", &Info::vm_peak},
     {Field::VM_SIZE, "VmSize", &Info::vm_size},
     {Field::VM_LCK, "VmLck", &Info::vm_lck},
     {Field::VM_PIN, "VmPin", &Info::vm_pin},
     {Field::VM_HWM, "VmHWM", &Info::vm_hwm},
     {Field::VM_RSS, "VmRSS", &Info::vm_rss},
     {Field::RSS_ANON, "RssAnon", &Info::rss_anon},
     {Field::RSS_FILE, "RssFile", &Info::rss_file},
     {Field::RSS_SHMEM, "RssShmem", &Info::rss_shmem},
     {Field::VM_DATA, "VmData", &Info::vm_data},
     {Field::VM_STK, "VmStk", &Info::vm_stk},
     {Field::VM_EXE, "VmExe", &Info::vm_exe},
     {Field::VM_LIB, "VmLib", &Info::vm_lib},
     {Field::VM_PTE, "VmPTE", &Info::vm_pte},
     {Field::VM_SWAP, "VmSwap", &Info::vm_swap}}};

Info Get(std::istream &is, const FieldMask &field_mask) {
    Info info;
    info.parsed |= kvparser.Parse(is, info, field_mask);
    return info;
}

Info Get() { return Get(~FieldMask{}); }
Info Get(pid_t pid) { return Get(pid, ~FieldMask{}); }

Info Get(const FieldMask &field_mask) {
    std::ifstream ifs("/proc/self/status");
    return Get(ifs, field_mask);
}

Info Get(pid_t pid, const FieldMask &field_mask) {
    std::ifstream ifs(fmt::format("/proc/{}/status", pid));
    return Get(ifs, field_mask);
}

std::ostream &operator<<(std::ostream &os, const Info &info) {
    kvparser.Format(os, info, info.parsed);
    return os;
}
} // namespace status
namespace maps {
std::uint32_t Vma::MajorDev() const { return major(dev); }
std::uint32_t Vma::MinorDev() const { return minor(dev); }

auto ParseVma(std::istream &is) {
    struct {
        explicit operator bool() const noexcept { return !failed; }
        Vma vma;
        bool failed{};
    } res;

    std::streampos checkpoint{is.tellg()};
    std::string buf;
    if (!std::getline(is, buf)) {
        res.failed = true;
        return res;
    }

    auto scan_res{scn::scan<uintptr_t, uintptr_t, char, char, char, char, off_t, uint32_t, uint32_t, ino_t>(
        buf, "{:x}-{:x} {}{}{}{} {:x} {:x}:{:x} {}")};
    if (!scan_res) {
        checkpoint = is.tellg();
        res.failed = true;
        return res;
    }
    checkpoint = is.tellg();

    const auto &values{scan_res->values()};
    res.vma.address.start = std::get<0>(values);
    res.vma.address.end = std::get<1>(values);

    res.vma.perms.r = (std::get<2>(values) == 'r');
    res.vma.perms.w = (std::get<3>(values) == 'w');
    res.vma.perms.x = (std::get<4>(values) == 'x');
    res.vma.perms.s = (std::get<5>(values) == 's');

    res.vma.offset = std::get<6>(values);
    res.vma.dev = makedev(std::get<7>(values), std::get<8>(values));
    res.vma.inode = std::get<9>(values);

    // scan剩余字符串即pathname
    auto range{scan_res->range()};
    res.vma.pathname = Strip({&*range.begin(), range.size()});
    return res;
}

void FormatVma(std::ostream &os, const Vma &vma) {
    char r{vma.perms.r ? 'r' : '-'};
    char w{vma.perms.w ? 'w' : '-'};
    char x{vma.perms.x ? 'x' : '-'};
    char p{vma.perms.s ? 's' : 'p'};

    std::string fmt_res{fmt::format(
        "{:x}-{:x} {}{}{}{} {:08x} {:02x}:{:02x} {}", vma.address.start, vma.address.end, r, w, x, p, vma.offset,
        vma.MajorDev(), vma.MinorDev(), vma.inode)};

    if (vma.pathname.empty()) {
        os << fmt_res;
    } else {
        os << fmt::format("{:<70}{}", fmt_res, vma.pathname);
    }
    os << "\n";
}

Info Get(std::istream &is) {
    Info info;
    while (auto res{maps::ParseVma(is)}) {
        info.vma_table.push_back(std::move(res.vma));
    }
    return info;
}

Info Get() {
    std::ifstream ifs("/proc/self/maps");
    return Get(ifs);
}

Info Get(pid_t pid) {
    std::ifstream ifs(fmt::format("/proc/{}/maps", pid));
    return Get(ifs);
}

std::ostream &operator<<(std::ostream &os, const Info &info) {
    for (const auto &vma : info.vma_table) {
        FormatVma(os, vma);
    }
    return os;
}
} // namespace maps
namespace smaps {
KeyValueParser<VmaExt, Field, TypeList<KiBs<std::size_t>, bool, decltype(VmaExt::vm_flags)>> kvparser{
    {{Field::SIZE, "Size", &VmaExt::size},
     {Field::KERNEL_PAGE_SIZE, "KernelPageSize", &VmaExt::kernel_page_size},
     {Field::MMU_PAGE_SIZE, "MMUPageSize", &VmaExt::mmu_page_size},
     {Field::RSS, "Rss", &VmaExt::rss},
     {Field::PSS, "Pss", &VmaExt::pss},
     {Field::PSS_DIRTY, "Pss_Dirty", &VmaExt::pss_dirty},
     {Field::SHARED_CLEAN, "Shared_Clean", &VmaExt::shared_clean},
     {Field::SHARED_DIRTY, "Shared_Dirty", &VmaExt::shared_dirty},
     {Field::PRIVATE_CLEAN, "Private_Clean", &VmaExt::private_clean},
     {Field::PRIVATE_DIRTY, "Private_Dirty", &VmaExt::private_dirty},
     {Field::REFERENCED, "Referenced", &VmaExt::referenced},
     {Field::ANONYMOUS, "Anonymous", &VmaExt::anonymous},
     {Field::KSM, "KSM", &VmaExt::ksm},
     {Field::LAZY_FREE, "LazyFree", &VmaExt::lazy_free},
     {Field::ANON_HUGE_PAGES, "AnonHugePages", &VmaExt::anon_huge_pages},
     {Field::SHMEM_PMD_MAPPED, "ShmemPmdMapped", &VmaExt::shmem_pmd_mapped},
     {Field::FILE_PMD_MAPPED, "FilePmdMapped", &VmaExt::file_pmd_mapped},
     {Field::SHARED_HUGETLB, "Shared_Hugetlb", &VmaExt::shared_hugetlb},
     {Field::PRIVATE_HUGETLB, "Private_Hugetlb", &VmaExt::private_hugetlb},
     {Field::SWAP, "Swap", &VmaExt::swap},
     {Field::SWAP_PSS, "SwapPss", &VmaExt::swap_pss},
     {Field::LOCKED, "Locked", &VmaExt::locked},
     {Field::THP_ELIGIBLE, "THPeligible", &VmaExt::thp_eligible},
     {Field::VM_FLAGS, "VmFlags", &VmaExt::vm_flags}},
    ":",
    [](std::string_view s) { return s.find('-') != std::string_view::npos; }};

Info Get(std::istream &is, const FieldMask &field_mask) {
    Info info;
    while (!is.eof()) {
        VmaExt vma_ext;
        if (field_mask.Test(Field::VMA)) {
            auto res{maps::ParseVma(is)};
            if (res) {
                vma_ext.vma = std::move(res.vma);
                vma_ext.parsed.Set(Field::VMA);
            }
        } else {
            is.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        }
        vma_ext.parsed |= kvparser.Parse(is, vma_ext, field_mask);
        info.vma_table.push_back(std::move(vma_ext));
    }
    return info;
}

Info Get() { return Get(~FieldMask{}); }
Info Get(pid_t pid) { return Get(pid, ~FieldMask{}); }

Info Get(const FieldMask &field_mask) {
    std::ifstream ifs("/proc/self/smaps");
    return Get(ifs, field_mask);
}

Info Get(pid_t pid, const FieldMask &field_mask) {
    std::ifstream ifs(fmt::format("/proc/{}/smaps", pid));
    return Get(ifs, field_mask);
}

std::ostream &operator<<(std::ostream &os, const Info &info) {
    for (const auto &vma_ext : info.vma_table) {
        if (vma_ext.parsed.Test(Field::VMA)) {
            maps::FormatVma(os, vma_ext.vma);
        }
        kvparser.Format(os, vma_ext, vma_ext.parsed);
    }
    return os;
}
} // namespace smaps
namespace smaps_rollup {
KeyValueParser<Info, Field, TypeList<KiBs<std::size_t>>> kvparser{
    {{Field::RSS, "Rss", &Info::rss},
     {Field::PSS, "Pss", &Info::pss},
     {Field::PSS_DIRTY, "Pss_Dirty", &Info::pss_dirty},
     {Field::PSS_ANON, "Pss_Anon", &Info::pss_anon},
     {Field::PSS_FILE, "Pss_File", &Info::pss_file},
     {Field::PSS_SHMEM, "Pss_Shmem", &Info::pss_shmem},
     {Field::SHARED_CLEAN, "Shared_Clean", &Info::shared_clean},
     {Field::SHARED_DIRTY, "Shared_Dirty", &Info::shared_dirty},
     {Field::PRIVATE_CLEAN, "Private_Clean", &Info::private_clean},
     {Field::PRIVATE_DIRTY, "Private_Dirty", &Info::private_dirty},
     {Field::REFERENCED, "Referenced", &Info::referenced},
     {Field::ANONYMOUS, "Anonymous", &Info::anonymous},
     {Field::KSM, "KSM", &Info::ksm},
     {Field::LAZY_FREE, "LazyFree", &Info::lazy_free},
     {Field::ANON_HUGE_PAGES, "AnonHugePages", &Info::anon_huge_pages},
     {Field::SHMEM_PMD_MAPPED, "ShmemPmdMapped", &Info::shmem_pmd_mapped},
     {Field::FILE_PMD_MAPPED, "FilePmdMapped", &Info::file_pmd_mapped},
     {Field::SHARED_HUGETLB, "Shared_Hugetlb", &Info::shared_hugetlb},
     {Field::PRIVATE_HUGETLB, "Private_Hugetlb", &Info::private_hugetlb},
     {Field::SWAP, "Swap", &Info::swap},
     {Field::SWAP_PSS, "SwapPss", &Info::swap_pss},
     {Field::LOCKED, "Locked", &Info::locked}}};

Info Get(std::istream &is, const FieldMask &field_mask) {
    Info info;
    if (field_mask.Test(Field::VMA)) {
        auto res{maps::ParseVma(is)};
        if (res) {
            info.vma = std::move(res.vma);
            info.parsed.Set(Field::VMA);
        }
    } else {
        // 跳过VMA行
        is.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    }
    info.parsed |= kvparser.Parse(is, info, field_mask);
    return info;
}

Info Get() { return Get(~FieldMask{}); }
Info Get(pid_t pid) { return Get(pid, ~FieldMask{}); }

Info Get(const FieldMask &field_mask) {
    std::ifstream ifs("/proc/self/smaps_rollup");
    return Get(ifs, field_mask);
}

Info Get(pid_t pid, const FieldMask &field_mask) {
    std::ifstream ifs(fmt::format("/proc/{}/smaps_rollup", pid));
    return Get(ifs, field_mask);
}

std::ostream &operator<<(std::ostream &os, const Info &info) {
    if (info.parsed.Test(Field::VMA)) {
        maps::FormatVma(os, info.vma);
    }
    kvparser.Format(os, info, info.parsed);
    return os;
}
} // namespace smaps_rollup
namespace numa_maps {
// TODO(resserops): 补充实现
} // namespace numa_maps
} // namespace proc
namespace lscpu {
KeyValueParser<Info, Field, TypeList<std::size_t, double, std::string>> kvparser{
    {{Field::ARCHITECTURE, "Architecture", &Info::architecture},
     {Field::CPUS, "CPU(s)", &Info::cpus},
     {Field::THREADS_PER_CORE, "Thread(s) per core", &Info::threads_per_core},
     {Field::CORES_PER_SOCKET, "Core(s) per socket", &Info::cores_per_socket},
     {Field::SOCKETS, "Socket(s)", &Info::sockets},
     {Field::NUMA_NODES, "NUMA node(s)", &Info::numa_nodes},
     {Field::MODEL_NAME, "Model name", &Info::model_name},
     {Field::CPU_MHZ, "CPU MHz", &Info::cpu_mhz}}};

Info Get() { return Get(~FieldMask{}); }
Info Get(const FieldMask &field_mask) {
    Info info;
    std::istringstream iss{GetCmdOutput("lscpu")};
    info.parsed |= kvparser.Parse(iss, info, field_mask);
    return info;
}

std::ostream &operator<<(std::ostream &os, const Info &info) {
    kvparser.Format(os, info, info.parsed);
    return os;
}
} // namespace lscpu
} // namespace oops
