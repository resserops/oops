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
// 获取命令输出
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

class IStreamBacktraceGuard {
public:
    explicit IStreamBacktraceGuard(std::istream &is) : is_{is}, pos_{is.tellg()} {}

    IStreamBacktraceGuard(const IStreamBacktraceGuard &) = delete;
    IStreamBacktraceGuard &operator=(const IStreamBacktraceGuard &) = delete;

    void Commit() { pos_ = is_.tellg(); }

    ~IStreamBacktraceGuard() {
        if (pos_ != std::streampos{-1}) {
            is_.clear();
            is_.seekg(pos_);
        }
    }

private:
    std::istream &is_;
    std::streampos pos_;
};

::std::string ToStr(const KiBs<std::size_t> &t) { return std::to_string(t.Count()); }

template <>
KiBs<std::size_t> FromStr(const ::std::string_view sv) {
    ::std::istringstream iss{::std::string{sv}}; // TODO(resserops): 优化自定义stream
    std::size_t t{};
    iss >> t;
    return KiBs<std::size_t>{t};
}

template <typename T>
bool FromSv(std::string_view sv, T &value) {
    value = FromStr<T>(sv);
    return true;
}

// 新版KeyValueParser
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
        // 后续如果没有特殊规则的解析，移除注册固化函数，或者使用fmt格式化字符串
        bool (*parse)(std::string_view, MemberPtrVar, Struct &){ParseField};
        std::string (*format)(MemberPtrVar, const Struct &){FormatField};
    };

    explicit KeyValueParser(std::initializer_list<Entry> entries) : field_table_{entries} {}
    KeyValueParser(std::initializer_list<Entry> entries, std::string delim)
        : field_table_{entries}, delim_{std::move(delim)} {}
    KeyValueParser(std::initializer_list<Entry> entries, std::function<bool(std::string_view)> stop)
        : field_table_{entries}, stop_{std::move(stop)} {}
    KeyValueParser(std::initializer_list<Entry> entries, std::string delim, std::function<bool(std::string_view)> stop)
        : field_table_{entries}, delim_{std::move(delim)}, stop_{std::move(stop)} {}

    static bool ParseField(std::string_view value, MemberPtrVar member_ptr_var, Struct &obj) {
        auto f = [value, &obj](auto member_ptr) {
            auto &member{obj.*member_ptr};
            return FromSv(value, member);
        };
        return std::visit(f, member_ptr_var);
    }

    static std::string FormatField(MemberPtrVar member_ptr_var, const Struct &obj) {
        auto f = [&obj](auto member_ptr) -> std::string {
            auto &member{obj.*member_ptr};
            return ToStr(member);
        };
        return std::visit(f, member_ptr_var);
    }

    // 根据注册表向object中各字段填值，更适合引用输出
    EnumBitset<Field> Parse(std::istream &is, Struct &object, const EnumBitset<Field> &field_mask) {
        IStreamBacktraceGuard guard(is);
        EnumBitset<Field> parsed;
        std::string line_buf;
        while (std::getline(is, line_buf)) {
            std::string_view line{line_buf};
            std::size_t pos{line.find(delim_)};
            if (stop_(line)) {
                break;
            }
            guard.Commit(); // 本行已经确定是KeyValuePair，提交本行

            if (pos == line.npos) {
                continue;
                ;
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

            it->parse(value, it->member_ptr_var, object);
            parsed |= it->field;
            guard.Commit();
            if (parsed == field_mask) {
                break; // 已解析全量
            }
        }
        return parsed;
    }

    void Format(std::ostream &os, const Struct &object, const EnumBitset<Field> &field_mask = ~EnumBitset<Field>{}) {
        FTable ftable;
        for (const auto &entry : field_table_) {
            if (field_mask.Test(entry.field)) {
                std::string value{entry.format(entry.member_ptr_var, object)};
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
    std::function<bool(std::string_view)> stop_{[](std::string_view) { return false; }};
};

namespace proc {
namespace status {
KeyValueParser<Info, Field, TypeList<std::size_t>> parser{
    {Field::VM_PEAK, "VmPeak", &Info::vm_peak},       {Field::VM_SIZE, "VmSize", &Info::vm_size},
    {Field::VM_LCK, "VmLck", &Info::vm_lck},          {Field::VM_PIN, "VmPin", &Info::vm_pin},
    {Field::VM_HWM, "VmHWM", &Info::vm_hwm},          {Field::VM_RSS, "VmRSS", &Info::vm_rss},
    {Field::RSS_ANON, "RssAnon", &Info::rss_anon},    {Field::RSS_FILE, "RssFile", &Info::rss_file},
    {Field::RSS_SHMEM, "RssShmem", &Info::rss_shmem}, {Field::VM_DATA, "VmData", &Info::vm_data},
    {Field::VM_STK, "VmStk", &Info::vm_stk},          {Field::VM_EXE, "VmExe", &Info::vm_exe},
    {Field::VM_LIB, "VmLib", &Info::vm_lib},          {Field::VM_PTE, "VmPTE", &Info::vm_pte},
    {Field::VM_SWAP, "VmSwap", &Info::vm_swap}};

Info Get(std::istream &is, const FieldMask &field_mask) {
    Info info;
    info.parsed |= parser.Parse(is, info, field_mask);
    return info;
}

Info Get() { return Get(~FieldMask{}); }
Info Get(pid_t pid) { return Get(pid, ~FieldMask{}); }

Info Get(const FieldMask &field_mask) {
    std::ifstream ifs("/proc/self/status");
    return Get(ifs, field_mask);
}

Info Get(pid_t pid, const FieldMask &field_mask) {
    std::ifstream ifs(std::string{"/proc/"} + std::to_string(pid) + "/status");
    return Get(ifs, field_mask);
}

std::ostream &operator<<(std::ostream &out, const Info &info) {
    parser.Format(out, info, info.parsed);
    return out;
}
} // namespace status

std::uint32_t Vma::MajorDev() const { return major(dev); }
std::uint32_t Vma::MinorDev() const { return minor(dev); }

auto ParseVma(std::istream &is) {
    IStreamBacktraceGuard guard(is);
    struct {
        explicit operator bool() const noexcept { return !failed; }
        Vma vma;
        bool failed{};
    } res;

    std::string buf;
    if (!std::getline(is, buf)) {
        res.failed = true;
        return res;
    }

    auto scan_res{scn::scan<uintptr_t, uintptr_t, char, char, char, char, off_t, uint32_t, uint32_t, ino_t>(
        buf, "{:x}-{:x} {}{}{}{} {:x} {:x}:{:x} {}")};
    if (!scan_res) {
        res.failed = true;
        return res;
    }
    guard.Commit();

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

namespace smaps_rollup {
KeyValueParser<Info, Field, TypeList<KiBs<std::size_t>>> parser{
    {Field::RSS, "Rss", &Info::rss},
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
    {Field::LOCKED, "Locked", &Info::locked}};

Info Get(std::istream &is, const FieldMask &field_mask) {
    Info info;
    if (field_mask.Test(Field::VMA)) {
        auto res{ParseVma(is)};
        if (res) {
            info.vma = std::move(res.vma);
            info.parsed.Set(Field::VMA);
        }
    } else {
        // 跳过VMA行
        is.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    }
    info.parsed |= parser.Parse(is, info, field_mask);
    return info;
}

Info Get() { return Get(~FieldMask{}); }
Info Get(pid_t pid) { return Get(pid, ~FieldMask{}); }

Info Get(const FieldMask &field_mask) {
    std::ifstream ifs("/proc/self/smaps_rollup");
    return Get(ifs, field_mask);
}

Info Get(pid_t pid, const FieldMask &field_mask) {
    std::ifstream ifs(std::string{"/proc/"} + std::to_string(pid) + "/smaps_rollup");
    return Get(ifs, field_mask);
}

std::ostream &operator<<(std::ostream &os, const Info &info) {
    if (info.parsed.Test(Field::VMA)) {
        FormatVma(os, info.vma);
    }
    parser.Format(os, info, info.parsed);
    return os;
}
} // namespace smaps_rollup
namespace numa_maps {
// TODO(resserops): 补充实现
} // namespace numa_maps
} // namespace proc

namespace lscpu {
KeyValueParser<Info, Field, TypeList<std::size_t, double, std::string>> parser{
    {Field::ARCHITECTURE, "Architecture", &Info::architecture},
    {Field::CPUS, "CPU(s)", &Info::cpus},
    {Field::THREADS_PER_CORE, "Thread(s) per core", &Info::threads_per_core},
    {Field::CORES_PER_SOCKET, "Core(s) per socket", &Info::cores_per_socket},
    {Field::SOCKETS, "Socket(s)", &Info::sockets},
    {Field::NUMA_NODES, "NUMA node(s)", &Info::numa_nodes},
    {Field::MODEL_NAME, "Model name", &Info::model_name},
    {Field::CPU_MHZ, "CPU MHz", &Info::cpu_mhz}};

Info Get() { return Get(~FieldMask{}); }
Info Get(const FieldMask &field_mask) {
    Info info;
    std::istringstream iss{GetCmdOutput("lscpu")};
    info.parsed |= parser.Parse(iss, info, field_mask);
    return info;
}

std::ostream &operator<<(std::ostream &out, const Info &info) {
    parser.Format(out, info, info.parsed);
    return out;
}
} // namespace lscpu
} // namespace oops
