#pragma once

#include <cstddef>
#include <cstdint>
#include <bitset>
#include <type_traits>
#include <optional>

#include "oops/enum_bitset.h"

namespace oops {
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
[[nodiscard]] Info Get(const FieldMask &field_mask);
::std::ostream& operator<<(::std::ostream &out, const Info &info);
}

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
::std::ostream& operator<<(::std::ostream &out, const Info &info);
}
}
