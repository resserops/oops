#include "oops/proc/task/smaps_rollup.h"

#include <fstream>
#include <istream>
#include <limits>
#include <ostream>
#include <utility>

#include "fmt/format.h"

#include "oops/key_value_parser.h"

namespace oops {
namespace proc {
namespace task {
namespace smaps_rollup {
namespace {
KeyValueParser<Info, Field> kvparser{
    {{Field::RSS, "Rss", CW<&Info::rss>, "{} kB"},
     {Field::PSS, "Pss", CW<&Info::pss>, "{} kB"},
     {Field::PSS_DIRTY, "Pss_Dirty", CW<&Info::pss_dirty>, "{} kB"},
     {Field::PSS_ANON, "Pss_Anon", CW<&Info::pss_anon>, "{} kB"},
     {Field::PSS_FILE, "Pss_File", CW<&Info::pss_file>, "{} kB"},
     {Field::PSS_SHMEM, "Pss_Shmem", CW<&Info::pss_shmem>, "{} kB"},
     {Field::SHARED_CLEAN, "Shared_Clean", CW<&Info::shared_clean>, "{} kB"},
     {Field::SHARED_DIRTY, "Shared_Dirty", CW<&Info::shared_dirty>, "{} kB"},
     {Field::PRIVATE_CLEAN, "Private_Clean", CW<&Info::private_clean>, "{} kB"},
     {Field::PRIVATE_DIRTY, "Private_Dirty", CW<&Info::private_dirty>, "{} kB"},
     {Field::REFERENCED, "Referenced", CW<&Info::referenced>, "{} kB"},
     {Field::ANONYMOUS, "Anonymous", CW<&Info::anonymous>, "{} kB"},
     {Field::KSM, "KSM", CW<&Info::ksm>, "{} kB"},
     {Field::LAZY_FREE, "LazyFree", CW<&Info::lazy_free>, "{} kB"},
     {Field::ANON_HUGE_PAGES, "AnonHugePages", CW<&Info::anon_huge_pages>, "{} kB"},
     {Field::SHMEM_PMD_MAPPED, "ShmemPmdMapped", CW<&Info::shmem_pmd_mapped>, "{} kB"},
     {Field::FILE_PMD_MAPPED, "FilePmdMapped", CW<&Info::file_pmd_mapped>, "{} kB"},
     {Field::SHARED_HUGETLB, "Shared_Hugetlb", CW<&Info::shared_hugetlb>, "{} kB"},
     {Field::PRIVATE_HUGETLB, "Private_Hugetlb", CW<&Info::private_hugetlb>, "{} kB"},
     {Field::SWAP, "Swap", CW<&Info::swap>, "{} kB"},
     {Field::SWAP_PSS, "SwapPss", CW<&Info::swap_pss>, "{} kB"},
     {Field::LOCKED, "Locked", CW<&Info::locked>, "{} kB"}}};
} // namespace

Info Get() { return Get(~FieldMask{}); }
Info Get(const FieldMask &field_mask) {
    std::ifstream ifs("/proc/self/smaps_rollup");
    return Get(ifs, field_mask);
}

Info Get(std::istream &is) { return Get(is, ~FieldMask{}); }
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
    info.parsed |= kvparser.Parse(is, info, field_mask);
    return info;
}

Info Get(pid_t pid) { return Get(pid, ~FieldMask{}); }
Info Get(pid_t pid, const FieldMask &field_mask) {
    std::ifstream ifs(fmt::format("/proc/{}/smaps_rollup", pid));
    return Get(ifs, field_mask);
}

Info Get(pid_t pid, pid_t tid) { return Get(pid, tid, ~FieldMask{}); }
Info Get(pid_t pid, pid_t tid, const FieldMask &field_mask) {
    std::ifstream ifs(fmt::format("/proc/{}/task/{}/smaps_rollup", pid, tid));
    return Get(ifs, field_mask);
}

std::ostream &operator<<(std::ostream &os, const Info &info) {
    if (info.parsed.Test(Field::VMA)) {
        FormatVma(os, info.vma);
    }
    kvparser.Format(os, info, info.parsed);
    return os;
}
} // namespace smaps_rollup
} // namespace task
} // namespace proc
} // namespace oops
