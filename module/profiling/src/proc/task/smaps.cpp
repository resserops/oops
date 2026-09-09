#include "oops/proc/task/smaps.h"

#include <algorithm>
#include <fstream>
#include <istream>
#include <limits>
#include <ostream>
#include <string>
#include <string_view>
#include <utility>

#include "fmt/format.h"

#include "oops/key_value_parser.h"
#include "oops/str.h"

namespace oops {
// proc::task::smaps::VmaExt::VmFlags特化
struct VmFlagsEntry {
    using VmFlags = proc::task::smaps::VmaExt::VmFlags;
    std::string_view key;
    void (*set)(VmFlags &);
    bool (*get)(const VmFlags &);
};

#define ENTRY(f)                                                       \
    {                                                                  \
        #f, [](auto &f) { f.rd = true; }, [](auto &f) { return f.rd; } \
    }
constexpr VmFlagsEntry VM_FLAGS_TABLE[]{ENTRY(rd), ENTRY(wr), ENTRY(ex), ENTRY(sh), ENTRY(mr), ENTRY(mw), ENTRY(me),
                                        ENTRY(ms), ENTRY(gd), ENTRY(pf), ENTRY(dw), ENTRY(lo), ENTRY(io), ENTRY(sr),
                                        ENTRY(rr), ENTRY(dc), ENTRY(de), ENTRY(ac), ENTRY(nr), ENTRY(ht), ENTRY(sf),
                                        ENTRY(nl), ENTRY(ar), ENTRY(wf), ENTRY(dd), ENTRY(sd), ENTRY(mm), ENTRY(hg),
                                        ENTRY(nh), ENTRY(mg), ENTRY(um), ENTRY(uw)};
#undef ENTRY

template <>
bool ParseField(std::string_view s, proc::task::smaps::VmaExt::VmFlags &vm_flags, std::string_view) {
    bool failed{false};
    for (auto token : Split(s)) {
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
std::string FormatField(const proc::task::smaps::VmaExt::VmFlags &vm_flags, std::string_view) {
    std::string s;
    s.reserve(64); // one cache line
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

namespace proc {
namespace task {
namespace smaps {
namespace {
KeyValueParser<VmaExt, Field, meta::TypeList<KiBs<std::size_t>, bool, decltype(VmaExt::vm_flags)>> kvparser{
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
} // namespace

Info Get() { return Get(~FieldMask{}); }
Info Get(const FieldMask &field_mask) {
    std::ifstream ifs("/proc/self/smaps");
    return Get(ifs, field_mask);
}

Info Get(std::istream &is) { return Get(is, ~FieldMask{}); }
Info Get(std::istream &is, const FieldMask &field_mask) {
    Info info;
    while (is.peek() != EOF) {
        VmaExt vma_ext;
        if (field_mask.Test(Field::VMA)) {
            auto res{ParseVma(is)};
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

Info Get(pid_t pid) { return Get(pid, ~FieldMask{}); }
Info Get(pid_t pid, const FieldMask &field_mask) {
    std::ifstream ifs(fmt::format("/proc/{}/smaps", pid));
    return Get(ifs, field_mask);
}

Info Get(pid_t pid, pid_t tid) { return Get(pid, tid, ~FieldMask{}); }
Info Get(pid_t pid, pid_t tid, const FieldMask &field_mask) {
    std::ifstream ifs(fmt::format("/proc/{}/task/{}/smaps", pid, tid));
    return Get(ifs, field_mask);
}

std::ostream &operator<<(std::ostream &os, const Info &info) {
    for (const auto &vma_ext : info.vma_table) {
        if (vma_ext.parsed.Test(Field::VMA)) {
            FormatVma(os, vma_ext.vma);
        }
        kvparser.Format(os, vma_ext, vma_ext.parsed);
    }
    return os;
}
} // namespace smaps
} // namespace task
} // namespace proc
} // namespace oops
