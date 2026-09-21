#pragma once
#include <cstddef>
#include <cstdint>
#include <iosfwd>

#include "oops/enum_bitset.h"
#include "oops/unit.h"

namespace oops {
namespace proc {
namespace meminfo {
enum class Field : uint8_t {
    MEM_TOTAL,
    MEM_FREE,
    MEM_AVAILABLE,
    BUFFERS,
    CACHED,
    SWAP_CACHED,
    ACTIVE,
    INACTIVE,
    ACTIVE_ANON,
    INACTIVE_ANON,
    ACTIVE_FILE,
    INACTIVE_FILE,
    UNEVICTABLE,
    MLOCKED,
    SWAP_TOTAL,
    SWAP_FREE,
    ZSWAP,
    ZSWAPPED,
    DIRTY,
    WRITEBACK,
    ANON_PAGES,
    MAPPED,
    SHMEM,
    KRECLAIMABLE,
    SLAB,
    SRECLAIMABLE,
    SUNRECLAIM,
    KERNEL_STACK,
    PAGE_TABLES,
    SEC_PAGE_TABLES,
    NFS_UNSTABLE,
    BOUNCE,
    WRITEBACK_TMP,
    COMMIT_LIMIT,
    COMMITTED_AS,
    VMALLOC_TOTAL,
    VMALLOC_USED,
    VMALLOC_CHUNK,
    PERCPU,
    HARDWARE_CORRUPTED,
    ANON_HUGE_PAGES,
    SHMEM_HUGE_PAGES,
    SHMEM_PMD_MAPPED,
    FILE_HUGE_PAGES,
    FILE_PMD_MAPPED,
    UNACCEPTED,
    HUGE_PAGES_TOTAL,
    HUGE_PAGES_FREE,
    HUGE_PAGES_RSVD,
    HUGE_PAGES_SURP,
    HUGE_PAGE_SIZE,
    HUGETLB,
    DIRECT_MAP_4K,
    DIRECT_MAP_2M,
    DIRECT_MAP_1G,
    COUNT
};
using FieldMask = EnumBitset<Field>;
using oops::operator|; // 支持Field和FieldMask或运算ADL

struct Info {
    // 基础信息
    KiBs<std::size_t> mem_total{};     // 总内存（不包含保留位和内核镜像）
    KiBs<std::size_t> mem_free{};      // 空闲内存
    KiBs<std::size_t> mem_available{}; // 可用内存估值（包含可回收内存）

    // 页面缓存
    KiBs<std::size_t> buffers{};     // 磁盘块元数据缓存
    KiBs<std::size_t> cached{};      // 页面缓存
    KiBs<std::size_t> swap_cached{}; // 交换区有副本的页面缓存

    // LRU
    KiBs<std::size_t> active{};        // 最近使用的活跃内存
    KiBs<std::size_t> inactive{};      // 非活跃内存
    KiBs<std::size_t> active_anon{};   // 活跃匿名映射
    KiBs<std::size_t> inactive_anon{}; // 非活跃匿名映射
    KiBs<std::size_t> active_file{};   // 活跃文件映射
    KiBs<std::size_t> inactive_file{}; // 非活跃文件映射
    KiBs<std::size_t> unevictable{};   // 不可回收内存
    KiBs<std::size_t> mlocked{};       // mlock锁定的内存

    // 交换区
    KiBs<std::size_t> swap_total{}; // 总交换空间
    KiBs<std::size_t> swap_free{};  // 空闲交换空间
    KiBs<std::size_t> zswap{};      // zswap交换空间（内存上的压缩形式交换空间缓存）
    KiBs<std::size_t> zswapped{};   // zswap中页面压缩前的大小

    // 页面统计
    KiBs<std::size_t> dirty{};      // 等待回写的脏页
    KiBs<std::size_t> writeback{};  // 正在回写的页面
    KiBs<std::size_t> anon_pages{}; // 私有匿名映射和COW文件映射
    KiBs<std::size_t> mapped{};     // 已映射的文件/tmpfs页面
    KiBs<std::size_t> shmem{};      // 所有tmpfs页面

    // 内核
    KiBs<std::size_t> kreclaimable{};    // 内核可回收内存
    KiBs<std::size_t> slab{};            // slab分配的内核数据结构缓存
    KiBs<std::size_t> sreclaimable{};    // 可回收的slab内存
    KiBs<std::size_t> sunreclaim{};      // 不可回收的slab内存
    KiBs<std::size_t> kernel_stack{};    // 内核栈
    KiBs<std::size_t> page_tables{};     // 用户态页表
    KiBs<std::size_t> sec_page_tables{}; // 次级页表（KVM影子页表等）

    // 临时缓冲区
    KiBs<std::size_t> nfs_unstable{};  // 已回写NFS服务器但未落盘的内存（脏页和洁净页中间状态）
    KiBs<std::size_t> bounce{};        // DMA设备寻址范围不够时，分配的低位中转缓冲区
    KiBs<std::size_t> writeback_tmp{}; // FUSE临时回写缓冲区

    // 提交
    KiBs<std::size_t> commit_limit{}; // 系统允许提交的内存总量（overcommit策略=2时生效）
    KiBs<std::size_t> committed_as{}; // 已提交的内存（包括未touch页面）

    // vmalloc
    KiBs<std::size_t> vmalloc_total{}; // 内核虚拟连续分配器vmalloc虚拟地址总量
    KiBs<std::size_t> vmalloc_used{};  // vmalloc已用内存
    KiBs<std::size_t> vmalloc_chunk{}; // vmalloc最大连续空闲虚拟地址

    // Per-CPU变量和损坏内存
    KiBs<std::size_t> percpu{};             // Per-CPU变量
    KiBs<std::size_t> hardware_corrupted{}; // 被隔离的物理损坏内存

    // 透明大页
    KiBs<std::size_t> anon_huge_pages{};  // 匿名透明大页
    KiBs<std::size_t> shmem_huge_pages{}; // shmem透明大页
    KiBs<std::size_t> shmem_pmd_mapped{}; // shmem PMD映射的透明大页
    KiBs<std::size_t> file_huge_pages{};  // 文件透明大页
    KiBs<std::size_t> file_pmd_mapped{};  // 文件PMD映射的透明大页

    // 内存加密虚拟机
    KiBs<std::size_t> unaccepted{}; // VMM已分配，但未被虚拟机接受（完整性校验）的内存

    // 标准大页
    std::size_t huge_pages_total{};     // 标准大页总数量
    std::size_t huge_pages_free{};      // 空闲标准大页
    std::size_t huge_pages_rsvd{};      // 已提交但未touch的标准大页
    std::size_t huge_pages_surp{};      // 超发标准大页（nr_hugepages配置外的）
    KiBs<std::size_t> huge_page_size{}; // 标准大页容量
    KiBs<std::size_t> hugetlb{};        // 标准大页总内存

    // 直接映射（x86架构）
    KiBs<std::size_t> direct_map_4k{}; // 页表4K标准页覆盖内存
    KiBs<std::size_t> direct_map_2m{}; // 页表2M大页覆盖内存
    KiBs<std::size_t> direct_map_1g{}; // 页表1G巨页覆盖内存

    FieldMask parsed;
};

[[nodiscard]] Info Get();
[[nodiscard]] Info Get(const FieldMask &field_mask);
[[nodiscard]] Info Get(std::istream &is);
[[nodiscard]] Info Get(std::istream &is, const FieldMask &field_mask);

std::ostream &operator<<(std::ostream &os, const Info &info);
} // namespace meminfo
} // namespace proc
} // namespace oops
