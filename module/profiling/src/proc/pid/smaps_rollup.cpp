#include "oops/proc/pid/smaps_rollup.h"

#include <fstream>
#include <istream>
#include <limits>
#include <ostream>
#include <utility>

#include "fmt/format.h"

#include "oops/key_value_parser.h"

namespace oops {
namespace proc {
namespace pid {
namespace smaps_rollup {
namespace {
KeyValueParser<Info, Field, meta::TypeList<KiBs<std::size_t>>> kvparser{
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
} // namespace

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
} // namespace pid
} // namespace proc
} // namespace oops
