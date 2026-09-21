#include "oops/proc/meminfo.h"

#include <fstream>
#include <istream>
#include <ostream>

#include "oops/key_value_parser.h"

namespace oops {
namespace proc {
namespace meminfo {
namespace {
using TL = meta::TypeList<KiBs, std::size_t>;
KeyValueParser<Info, Field, TL> kvparser{
    {{Field::MEM_TOTAL, "MemTotal", &Info::mem_total, "{} kB"},
     {Field::MEM_FREE, "MemFree", &Info::mem_free, "{} kB"},
     {Field::MEM_AVAILABLE, "MemAvailable", &Info::mem_available, "{} kB"},
     {Field::BUFFERS, "Buffers", &Info::buffers, "{} kB"},
     {Field::CACHED, "Cached", &Info::cached, "{} kB"},
     {Field::SWAP_CACHED, "SwapCached", &Info::swap_cached, "{} kB"},
     {Field::ACTIVE, "Active", &Info::active, "{} kB"},
     {Field::INACTIVE, "Inactive", &Info::inactive, "{} kB"},
     {Field::ACTIVE_ANON, "Active(anon)", &Info::active_anon, "{} kB"},
     {Field::INACTIVE_ANON, "Inactive(anon)", &Info::inactive_anon, "{} kB"},
     {Field::ACTIVE_FILE, "Active(file)", &Info::active_file, "{} kB"},
     {Field::INACTIVE_FILE, "Inactive(file)", &Info::inactive_file, "{} kB"},
     {Field::UNEVICTABLE, "Unevictable", &Info::unevictable, "{} kB"},
     {Field::MLOCKED, "Mlocked", &Info::mlocked, "{} kB"},
     {Field::SWAP_TOTAL, "SwapTotal", &Info::swap_total, "{} kB"},
     {Field::SWAP_FREE, "SwapFree", &Info::swap_free, "{} kB"},
     {Field::ZSWAP, "Zswap", &Info::zswap, "{} kB"},
     {Field::ZSWAPPED, "Zswapped", &Info::zswapped, "{} kB"},
     {Field::DIRTY, "Dirty", &Info::dirty, "{} kB"},
     {Field::WRITEBACK, "Writeback", &Info::writeback, "{} kB"},
     {Field::ANON_PAGES, "AnonPages", &Info::anon_pages, "{} kB"},
     {Field::MAPPED, "Mapped", &Info::mapped, "{} kB"},
     {Field::SHMEM, "Shmem", &Info::shmem, "{} kB"},
     {Field::KRECLAIMABLE, "KReclaimable", &Info::kreclaimable, "{} kB"},
     {Field::SLAB, "Slab", &Info::slab, "{} kB"},
     {Field::SRECLAIMABLE, "SReclaimable", &Info::sreclaimable, "{} kB"},
     {Field::SUNRECLAIM, "SUnreclaim", &Info::sunreclaim, "{} kB"},
     {Field::KERNEL_STACK, "KernelStack", &Info::kernel_stack, "{} kB"},
     {Field::PAGE_TABLES, "PageTables", &Info::page_tables, "{} kB"},
     {Field::SEC_PAGE_TABLES, "SecPageTables", &Info::sec_page_tables, "{} kB"},
     {Field::NFS_UNSTABLE, "NFS_Unstable", &Info::nfs_unstable, "{} kB"},
     {Field::BOUNCE, "Bounce", &Info::bounce, "{} kB"},
     {Field::WRITEBACK_TMP, "WritebackTmp", &Info::writeback_tmp, "{} kB"},
     {Field::COMMIT_LIMIT, "CommitLimit", &Info::commit_limit, "{} kB"},
     {Field::COMMITTED_AS, "Committed_AS", &Info::committed_as, "{} kB"},
     {Field::VMALLOC_TOTAL, "VmallocTotal", &Info::vmalloc_total, "{} kB"},
     {Field::VMALLOC_USED, "VmallocUsed", &Info::vmalloc_used, "{} kB"},
     {Field::VMALLOC_CHUNK, "VmallocChunk", &Info::vmalloc_chunk, "{} kB"},
     {Field::PERCPU, "Percpu", &Info::percpu, "{} kB"},
     {Field::HARDWARE_CORRUPTED, "HardwareCorrupted", &Info::hardware_corrupted, "{} kB"},
     {Field::ANON_HUGE_PAGES, "AnonHugePages", &Info::anon_huge_pages, "{} kB"},
     {Field::SHMEM_HUGE_PAGES, "ShmemHugePages", &Info::shmem_huge_pages, "{} kB"},
     {Field::SHMEM_PMD_MAPPED, "ShmemPmdMapped", &Info::shmem_pmd_mapped, "{} kB"},
     {Field::FILE_HUGE_PAGES, "FileHugePages", &Info::file_huge_pages, "{} kB"},
     {Field::FILE_PMD_MAPPED, "FilePmdMapped", &Info::file_pmd_mapped, "{} kB"},
     {Field::UNACCEPTED, "Unaccepted", &Info::unaccepted, "{} kB"},
     {Field::HUGE_PAGES_TOTAL, "HugePages_Total", &Info::huge_pages_total},
     {Field::HUGE_PAGES_FREE, "HugePages_Free", &Info::huge_pages_free},
     {Field::HUGE_PAGES_RSVD, "HugePages_Rsvd", &Info::huge_pages_rsvd},
     {Field::HUGE_PAGES_SURP, "HugePages_Surp", &Info::huge_pages_surp},
     {Field::HUGE_PAGE_SIZE, "Hugepagesize", &Info::huge_page_size, "{} kB"},
     {Field::HUGETLB, "Hugetlb", &Info::hugetlb, "{} kB"},
     {Field::DIRECT_MAP_4K, "DirectMap4k", &Info::direct_map_4k, "{} kB"},
     {Field::DIRECT_MAP_2M, "DirectMap2M", &Info::direct_map_2m, "{} kB"},
     {Field::DIRECT_MAP_1G, "DirectMap1G", &Info::direct_map_1g, "{} kB"}}};
} // namespace

Info Get() { return Get(~FieldMask{}); }
Info Get(const FieldMask &field_mask) {
    std::ifstream ifs("/proc/meminfo");
    return Get(ifs, field_mask);
}

Info Get(std::istream &is) { return Get(is, ~FieldMask{}); }
Info Get(std::istream &is, const FieldMask &field_mask) {
    Info info;
    info.parsed |= kvparser.Parse(is, info, field_mask);
    return info;
}

std::ostream &operator<<(std::ostream &os, const Info &info) {
    kvparser.Format(os, info, info.parsed);
    return os;
}
} // namespace meminfo
} // namespace proc
} // namespace oops
