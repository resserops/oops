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

#include "oops/key_value_io.h"
#include "oops/str.h"

namespace oops {
namespace proc {
namespace task {
namespace smaps {
namespace {
struct VmFlagsEntry {
    using VmFlags = VmaExt::VmFlags;
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

bool ScanVmFlags(std::string_view s, VmaExt::VmFlags &vm_flags) {
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

std::string FormatVmFlags(const VmaExt::VmFlags &vm_flags) {
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

KeyValueIO<VmaExt, Field> kvio{
    {{Field::SIZE, "Size", CW<&VmaExt::size>, "{} kB"},
     {Field::KERNEL_PAGE_SIZE, "KernelPageSize", CW<&VmaExt::kernel_page_size>, "{} kB"},
     {Field::MMU_PAGE_SIZE, "MMUPageSize", CW<&VmaExt::mmu_page_size>, "{} kB"},
     {Field::RSS, "Rss", CW<&VmaExt::rss>, "{} kB"},
     {Field::PSS, "Pss", CW<&VmaExt::pss>, "{} kB"},
     {Field::PSS_DIRTY, "Pss_Dirty", CW<&VmaExt::pss_dirty>, "{} kB"},
     {Field::SHARED_CLEAN, "Shared_Clean", CW<&VmaExt::shared_clean>, "{} kB"},
     {Field::SHARED_DIRTY, "Shared_Dirty", CW<&VmaExt::shared_dirty>, "{} kB"},
     {Field::PRIVATE_CLEAN, "Private_Clean", CW<&VmaExt::private_clean>, "{} kB"},
     {Field::PRIVATE_DIRTY, "Private_Dirty", CW<&VmaExt::private_dirty>, "{} kB"},
     {Field::REFERENCED, "Referenced", CW<&VmaExt::referenced>, "{} kB"},
     {Field::ANONYMOUS, "Anonymous", CW<&VmaExt::anonymous>, "{} kB"},
     {Field::KSM, "KSM", CW<&VmaExt::ksm>, "{} kB"},
     {Field::LAZY_FREE, "LazyFree", CW<&VmaExt::lazy_free>, "{} kB"},
     {Field::ANON_HUGE_PAGES, "AnonHugePages", CW<&VmaExt::anon_huge_pages>, "{} kB"},
     {Field::SHMEM_PMD_MAPPED, "ShmemPmdMapped", CW<&VmaExt::shmem_pmd_mapped>, "{} kB"},
     {Field::FILE_PMD_MAPPED, "FilePmdMapped", CW<&VmaExt::file_pmd_mapped>, "{} kB"},
     {Field::SHARED_HUGETLB, "Shared_Hugetlb", CW<&VmaExt::shared_hugetlb>, "{} kB"},
     {Field::PRIVATE_HUGETLB, "Private_Hugetlb", CW<&VmaExt::private_hugetlb>, "{} kB"},
     {Field::SWAP, "Swap", CW<&VmaExt::swap>, "{} kB"},
     {Field::SWAP_PSS, "SwapPss", CW<&VmaExt::swap_pss>, "{} kB"},
     {Field::LOCKED, "Locked", CW<&VmaExt::locked>, "{} kB"},
     {Field::THP_ELIGIBLE, "THPeligible", CW<&VmaExt::thp_eligible>},
     {Field::VM_FLAGS, "VmFlags", CW<&VmaExt::vm_flags>, CW<ScanVmFlags>, CW<FormatVmFlags>}},
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
            auto res{ScanVma(is)};
            if (res) {
                vma_ext.vma = std::move(res.vma);
                vma_ext.parsed.Set(Field::VMA);
            }
        } else {
            is.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        }
        vma_ext.parsed |= kvio.Scan(is, vma_ext, field_mask);
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
        kvio.Format(os, vma_ext, vma_ext.parsed);
    }
    return os;
}
} // namespace smaps
} // namespace task
} // namespace proc
} // namespace oops
