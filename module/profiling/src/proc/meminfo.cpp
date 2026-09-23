#include "oops/proc/meminfo.h"

#include <fstream>
#include <istream>
#include <ostream>

#include "oops/key_value_parser.h"

namespace oops {
namespace proc {
namespace meminfo {
namespace {
KeyValueParser<Info, Field> kvparser{
    {{Field::MEM_TOTAL, "MemTotal", CW<&Info::mem_total>, "{} kB"},
     {Field::MEM_FREE, "MemFree", CW<&Info::mem_free>, "{} kB"},
     {Field::MEM_AVAILABLE, "MemAvailable", CW<&Info::mem_available>, "{} kB"},
     {Field::BUFFERS, "Buffers", CW<&Info::buffers>, "{} kB"},
     {Field::CACHED, "Cached", CW<&Info::cached>, "{} kB"},
     {Field::SWAP_CACHED, "SwapCached", CW<&Info::swap_cached>, "{} kB"},
     {Field::ACTIVE, "Active", CW<&Info::active>, "{} kB"},
     {Field::INACTIVE, "Inactive", CW<&Info::inactive>, "{} kB"},
     {Field::ACTIVE_ANON, "Active(anon)", CW<&Info::active_anon>, "{} kB"},
     {Field::INACTIVE_ANON, "Inactive(anon)", CW<&Info::inactive_anon>, "{} kB"},
     {Field::ACTIVE_FILE, "Active(file)", CW<&Info::active_file>, "{} kB"},
     {Field::INACTIVE_FILE, "Inactive(file)", CW<&Info::inactive_file>, "{} kB"},
     {Field::UNEVICTABLE, "Unevictable", CW<&Info::unevictable>, "{} kB"},
     {Field::MLOCKED, "Mlocked", CW<&Info::mlocked>, "{} kB"},
     {Field::SWAP_TOTAL, "SwapTotal", CW<&Info::swap_total>, "{} kB"},
     {Field::SWAP_FREE, "SwapFree", CW<&Info::swap_free>, "{} kB"},
     {Field::ZSWAP, "Zswap", CW<&Info::zswap>, "{} kB"},
     {Field::ZSWAPPED, "Zswapped", CW<&Info::zswapped>, "{} kB"},
     {Field::DIRTY, "Dirty", CW<&Info::dirty>, "{} kB"},
     {Field::WRITEBACK, "Writeback", CW<&Info::writeback>, "{} kB"},
     {Field::ANON_PAGES, "AnonPages", CW<&Info::anon_pages>, "{} kB"},
     {Field::MAPPED, "Mapped", CW<&Info::mapped>, "{} kB"},
     {Field::SHMEM, "Shmem", CW<&Info::shmem>, "{} kB"},
     {Field::KRECLAIMABLE, "KReclaimable", CW<&Info::kreclaimable>, "{} kB"},
     {Field::SLAB, "Slab", CW<&Info::slab>, "{} kB"},
     {Field::SRECLAIMABLE, "SReclaimable", CW<&Info::sreclaimable>, "{} kB"},
     {Field::SUNRECLAIM, "SUnreclaim", CW<&Info::sunreclaim>, "{} kB"},
     {Field::KERNEL_STACK, "KernelStack", CW<&Info::kernel_stack>, "{} kB"},
     {Field::PAGE_TABLES, "PageTables", CW<&Info::page_tables>, "{} kB"},
     {Field::SEC_PAGE_TABLES, "SecPageTables", CW<&Info::sec_page_tables>, "{} kB"},
     {Field::NFS_UNSTABLE, "NFS_Unstable", CW<&Info::nfs_unstable>, "{} kB"},
     {Field::BOUNCE, "Bounce", CW<&Info::bounce>, "{} kB"},
     {Field::WRITEBACK_TMP, "WritebackTmp", CW<&Info::writeback_tmp>, "{} kB"},
     {Field::COMMIT_LIMIT, "CommitLimit", CW<&Info::commit_limit>, "{} kB"},
     {Field::COMMITTED_AS, "Committed_AS", CW<&Info::committed_as>, "{} kB"},
     {Field::VMALLOC_TOTAL, "VmallocTotal", CW<&Info::vmalloc_total>, "{} kB"},
     {Field::VMALLOC_USED, "VmallocUsed", CW<&Info::vmalloc_used>, "{} kB"},
     {Field::VMALLOC_CHUNK, "VmallocChunk", CW<&Info::vmalloc_chunk>, "{} kB"},
     {Field::PERCPU, "Percpu", CW<&Info::percpu>, "{} kB"},
     {Field::HARDWARE_CORRUPTED, "HardwareCorrupted", CW<&Info::hardware_corrupted>, "{} kB"},
     {Field::ANON_HUGE_PAGES, "AnonHugePages", CW<&Info::anon_huge_pages>, "{} kB"},
     {Field::SHMEM_HUGE_PAGES, "ShmemHugePages", CW<&Info::shmem_huge_pages>, "{} kB"},
     {Field::SHMEM_PMD_MAPPED, "ShmemPmdMapped", CW<&Info::shmem_pmd_mapped>, "{} kB"},
     {Field::FILE_HUGE_PAGES, "FileHugePages", CW<&Info::file_huge_pages>, "{} kB"},
     {Field::FILE_PMD_MAPPED, "FilePmdMapped", CW<&Info::file_pmd_mapped>, "{} kB"},
     {Field::UNACCEPTED, "Unaccepted", CW<&Info::unaccepted>, "{} kB"},
     {Field::HUGE_PAGES_TOTAL, "HugePages_Total", CW<&Info::huge_pages_total>},
     {Field::HUGE_PAGES_FREE, "HugePages_Free", CW<&Info::huge_pages_free>},
     {Field::HUGE_PAGES_RSVD, "HugePages_Rsvd", CW<&Info::huge_pages_rsvd>},
     {Field::HUGE_PAGES_SURP, "HugePages_Surp", CW<&Info::huge_pages_surp>},
     {Field::HUGE_PAGE_SIZE, "Hugepagesize", CW<&Info::huge_page_size>, "{} kB"},
     {Field::HUGETLB, "Hugetlb", CW<&Info::hugetlb>, "{} kB"},
     {Field::DIRECT_MAP_4K, "DirectMap4k", CW<&Info::direct_map_4k>, "{} kB"},
     {Field::DIRECT_MAP_2M, "DirectMap2M", CW<&Info::direct_map_2m>, "{} kB"},
     {Field::DIRECT_MAP_1G, "DirectMap1G", CW<&Info::direct_map_1g>, "{} kB"}}};
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
