#pragma once

#include <cstdint>
#include <iosfwd>

#include "oops/enum_bitset.h"
#include "oops/proc/pid/maps.h"
#include "oops/unit.h"

namespace oops {
namespace proc {
namespace pid {
namespace smaps_rollup {
// Since Linux
// clang-format off
enum class Field : uint8_t {
    VMA,              RSS,              PSS,              PSS_DIRTY,        PSS_ANON,         PSS_FILE,
    PSS_SHMEM,        SHARED_CLEAN,     SHARED_DIRTY,     PRIVATE_CLEAN,    PRIVATE_DIRTY,    REFERENCED,
    ANONYMOUS,        KSM,              LAZY_FREE,        ANON_HUGE_PAGES,  SHMEM_PMD_MAPPED, FILE_PMD_MAPPED,
    SHARED_HUGETLB,   PRIVATE_HUGETLB,  SWAP,             SWAP_PSS,         LOCKED,           COUNT
};
// clang-format on
using FieldMask = EnumBitset<Field>;
using oops::operator|; // 支持Field和FieldMask或运算ADL

using Vma = maps::Vma;
struct Info {
    Vma vma;
    // 核心内存
    KiBs<std::size_t> rss{};
    KiBs<std::size_t> pss{};
    KiBs<std::size_t> pss_dirty{};
    KiBs<std::size_t> pss_anon{};
    KiBs<std::size_t> pss_file{};
    KiBs<std::size_t> pss_shmem{};

    // 页面共享状态
    KiBs<std::size_t> shared_clean{};
    KiBs<std::size_t> shared_dirty{};
    KiBs<std::size_t> private_clean{};
    KiBs<std::size_t> private_dirty{};

    // 引用与内核
    KiBs<std::size_t> referenced{};
    KiBs<std::size_t> anonymous{};
    KiBs<std::size_t> ksm{};
    KiBs<std::size_t> lazy_free{};

    // 大页
    KiBs<std::size_t> anon_huge_pages{};
    KiBs<std::size_t> shmem_pmd_mapped{};
    KiBs<std::size_t> file_pmd_mapped{};
    KiBs<std::size_t> shared_hugetlb{};
    KiBs<std::size_t> private_hugetlb{};

    // 交换区与锁定
    KiBs<std::size_t> swap{};
    KiBs<std::size_t> swap_pss{};
    KiBs<std::size_t> locked{};

    FieldMask parsed;
};

[[nodiscard]] Info Get();
[[nodiscard]] Info Get(pid_t pid);
[[nodiscard]] Info Get(const FieldMask &field_mask);
[[nodiscard]] Info Get(pid_t pid, const FieldMask &field_mask);
std::ostream &operator<<(std::ostream &os, const Info &info);
} // namespace smaps_rollup
} // namespace pid
} // namespace proc
} // namespace oops
