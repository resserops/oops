#pragma once
#include <cstdint>
#include <iosfwd>

#include "oops/enum_bitset.h"
#include "oops/proc/task/vma.h"
#include "oops/storage.h"

namespace oops {
namespace proc {
namespace task {
namespace smaps_rollup {
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

struct Info {
    Vma vma;
    // 核心内存
    KiBs rss{};
    KiBs pss{};
    KiBs pss_dirty{};
    KiBs pss_anon{};
    KiBs pss_file{};
    KiBs pss_shmem{};

    // 页面共享状态
    KiBs shared_clean{};
    KiBs shared_dirty{};
    KiBs private_clean{};
    KiBs private_dirty{};

    // 引用与内核
    KiBs referenced{};
    KiBs anonymous{};
    KiBs ksm{};
    KiBs lazy_free{};

    // 大页
    KiBs anon_huge_pages{};
    KiBs shmem_pmd_mapped{};
    KiBs file_pmd_mapped{};
    KiBs shared_hugetlb{};
    KiBs private_hugetlb{};

    // 交换区与锁定
    KiBs swap{};
    KiBs swap_pss{};
    KiBs locked{};

    FieldMask parsed;
};

[[nodiscard]] Info Get();
[[nodiscard]] Info Get(const FieldMask &field_mask);
[[nodiscard]] Info Get(std::istream &is);
[[nodiscard]] Info Get(std::istream &is, const FieldMask &field_mask);
[[nodiscard]] Info Get(pid_t pid);                                         // /proc/{pid}
[[nodiscard]] Info Get(pid_t pid, const FieldMask &field_mask);            // /proc/{pid}
[[nodiscard]] Info Get(pid_t pid, pid_t tid);                              // /proc/{pid}/task/{tid}
[[nodiscard]] Info Get(pid_t pid, pid_t tid, const FieldMask &field_mask); // /proc/{pid}/task/{tid}
std::ostream &operator<<(std::ostream &os, const Info &info);
} // namespace smaps_rollup
} // namespace task
} // namespace proc
} // namespace oops
