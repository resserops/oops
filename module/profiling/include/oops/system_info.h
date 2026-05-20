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
enum class Field : uint8_t {
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

struct Info {
    std::size_t vm_peak{}; // 当前只解析内存部分
    std::size_t vm_size{};
    std::size_t vm_lck{};
    std::size_t vm_pin{};
    std::size_t vm_hwm{};
    std::size_t vm_rss{};
    std::size_t rss_anon{};
    std::size_t rss_file{};
    std::size_t rss_shmem{};
    std::size_t vm_data{};
    std::size_t vm_stk{};
    std::size_t vm_exe{};
    std::size_t vm_lib{};
    std::size_t vm_pte{};
    std::size_t vm_swap{};
    FieldMask parsed;
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
    bool heap;
    bool stack;
    bool huge;
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
    std::string architecture{};
    std::size_t cpus{};
    std::size_t threads_per_core{};
    std::size_t cores_per_socket{};
    std::size_t sockets{};
    std::size_t numa_nodes{};
    std::string model_name{};
    double cpu_mhz{};
    FieldMask parsed;
};

[[nodiscard]] Info Get();
[[nodiscard]] Info Get(const FieldMask &field_mask);
::std::ostream &operator<<(::std::ostream &out, const Info &info);
} // namespace lscpu
} // namespace oops
