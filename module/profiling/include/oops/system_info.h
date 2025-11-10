#pragma once

#include <bitset>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <type_traits>
#include <vector>

#include "oops/enum_bitset.h"

namespace oops {
namespace proc {
namespace status {
enum class Field {
    VM_PEAK,
    VM_SIZE,
    VM_LCK,
    VM_PIN,
    VM_HWM,
    VM_RSS,
    RSS_ANON,
    RSS_FILE,
    RSS_SHMEM,
    VM_DATA,
    VM_STK,
    VM_EXE,
    VM_LIB,
    VM_PTE,
    VM_SWAP,
    COUNT
};
using FieldMask = EnumBitset<Field>;
using oops::operator|; // 支持Field和FieldMask或运算ADL

// 当前只解析内存部分
struct Info {
    std::optional<size_t> vm_peak;
    std::optional<size_t> vm_size;
    std::optional<size_t> vm_lck;
    std::optional<size_t> vm_pin;
    std::optional<size_t> vm_hwm;
    std::optional<size_t> vm_rss;
    std::optional<size_t> rss_anon;
    std::optional<size_t> rss_file;
    std::optional<size_t> rss_shmem;
    std::optional<size_t> vm_data;
    std::optional<size_t> vm_stk;
    std::optional<size_t> vm_exe;
    std::optional<size_t> vm_lib;
    std::optional<size_t> vm_pte;
    std::optional<size_t> vm_swap;
};

[[nodiscard]] Info Get();
[[nodiscard]] Info Get(int pid);
[[nodiscard]] Info Get(const FieldMask &field_mask);
[[nodiscard]] Info Get(int pid, const FieldMask &field_mask);
::std::ostream &operator<<(::std::ostream &out, const Info &info);
} // namespace status
namespace numa_maps {
// Since Linux 2.6.14
struct MemoryRange {
    uintptr_t start_addr;
    std::string memory_policy;
    std::vector<std::size_t> n_nodes; // nr_pages
    std::optional<std::string> file;  // filename
    // 暂用opt<mono>表达一个key有或无的二元状态，相比bool形式上更加统一
    std::optional<std::monostate> heap;
    std::optional<std::monostate> stack;
    std::optional<std::monostate> huge;
    std::optional<std::size_t> anon;
    std::optional<std::size_t> dirty;
    std::optional<std::size_t> mapped;    // pages
    std::optional<std::size_t> mapmax;    // count
    std::optional<std::size_t> swapcache; // count
    std::optional<std::size_t> active;    // pages
    std::optional<std::size_t> writeback; // pages
};

struct Info {
    std::vector<MemoryRange> memory_ranges;
};

[[nodiscard]] Info Get();
} // namespace numa_maps
} // namespace proc

namespace lscpu {
enum class Field {
    ARCHITECTURE,
    CPUS,
    THREADS_PER_CORE,
    CORES_PER_SOCKET,
    SOCKETS,
    NUMA_NODES,
    MODEL_NAME,
    CPU_MHZ,
    COUNT
};
using FieldMask = EnumBitset<Field>;
using oops::operator|; // 支持Field和FieldMask或运算ADL

struct Info {
    std::optional<std::string> architecture;
    std::optional<size_t> cpus;
    std::optional<size_t> threads_per_core;
    std::optional<size_t> cores_per_socket;
    std::optional<size_t> sockets;
    std::optional<size_t> numa_nodes;
    std::optional<std::string> model_name;
    std::optional<double> cpu_mhz;
};

[[nodiscard]] Info Get();
[[nodiscard]] Info Get(const FieldMask &field_mask);
::std::ostream &operator<<(::std::ostream &out, const Info &info);
} // namespace lscpu
} // namespace oops
