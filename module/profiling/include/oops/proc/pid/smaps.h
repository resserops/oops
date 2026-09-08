#pragma once

#include <cstdint>
#include <iosfwd>

#include "oops/enum_bitset.h"
#include "oops/proc/pid/maps.h"
#include "oops/unit.h"

namespace oops {
namespace proc {
namespace pid {
namespace smaps {
// clang-format off
enum class Field : uint8_t {
    VMA,              SIZE,             KERNEL_PAGE_SIZE, MMU_PAGE_SIZE,    RSS,              PSS,
    PSS_DIRTY,        SHARED_CLEAN,     SHARED_DIRTY,     PRIVATE_CLEAN,    PRIVATE_DIRTY,    REFERENCED,
    ANONYMOUS,        KSM,              LAZY_FREE,        ANON_HUGE_PAGES,  SHMEM_PMD_MAPPED, FILE_PMD_MAPPED,
    SHARED_HUGETLB,   PRIVATE_HUGETLB,  SWAP,             SWAP_PSS,         LOCKED,           THP_ELIGIBLE,
    VM_FLAGS,         COUNT
};
// clang-format on
using FieldMask = EnumBitset<Field>;
using oops::operator|;

using Vma = maps::Vma;
struct VmaExt {
    Vma vma;
    // 核心内存统计
    KiBs<std::size_t> size{};
    KiBs<std::size_t> kernel_page_size{};
    KiBs<std::size_t> mmu_page_size{};
    KiBs<std::size_t> rss{};
    KiBs<std::size_t> pss{};
    KiBs<std::size_t> pss_dirty{};

    // 页面共享状态统计
    KiBs<std::size_t> shared_clean{};
    KiBs<std::size_t> shared_dirty{};
    KiBs<std::size_t> private_clean{};
    KiBs<std::size_t> private_dirty{};

    // 引用与内核统计
    KiBs<std::size_t> referenced{};
    KiBs<std::size_t> anonymous{};
    KiBs<std::size_t> ksm{};
    KiBs<std::size_t> lazy_free{};

    // 大页统计
    KiBs<std::size_t> anon_huge_pages{};
    KiBs<std::size_t> shmem_pmd_mapped{};
    KiBs<std::size_t> file_pmd_mapped{};
    KiBs<std::size_t> shared_hugetlb{};
    KiBs<std::size_t> private_hugetlb{};

    // 交换区与锁定统计
    KiBs<std::size_t> swap{};
    KiBs<std::size_t> swap_pss{};
    KiBs<std::size_t> locked{};

    // 标志统计
    bool thp_eligible{};
    struct VmFlags {
        bool rd : 1;
        bool wr : 1;
        bool ex : 1;
        bool sh : 1;
        bool mr : 1;
        bool mw : 1;
        bool me : 1;
        bool ms : 1;
        bool gd : 1;
        bool pf : 1;
        bool dw : 1;
        bool lo : 1;
        bool io : 1;
        bool sr : 1;
        bool rr : 1;
        bool dc : 1;
        bool de : 1;
        bool ac : 1;
        bool nr : 1;
        bool ht : 1;
        bool sf : 1;
        bool nl : 1;
        bool ar : 1;
        bool wf : 1;
        bool dd : 1;
        bool sd : 1;
        bool mm : 1;
        bool hg : 1;
        bool nh : 1;
        bool mg : 1;
        bool um : 1;
        bool uw : 1;
    } vm_flags{};

    FieldMask parsed;
};

struct Info {
    std::vector<VmaExt> vma_table;
};

[[nodiscard]] Info Get();
[[nodiscard]] Info Get(pid_t pid);
[[nodiscard]] Info Get(const FieldMask &field_mask);
[[nodiscard]] Info Get(pid_t pid, const FieldMask &field_mask);
std::ostream &operator<<(std::ostream &os, const Info &info);
} // namespace smaps
} // namespace pid
} // namespace proc
} // namespace oops
