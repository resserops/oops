#pragma once

#include <cstddef>
#include <cstdint>
#include <bitset>

namespace oops {
namespace status {
// 当前只解析内存部分
struct Info {
    size_t vm_peak{};
    size_t vm_size{};
    size_t vm_lck{};
    size_t vm_pin{};
    size_t vm_hwm{};
    size_t vm_rss{};
    size_t rss_anon{};
    size_t rss_file{};
    size_t rss_shmem{};
    size_t vm_data{};
    size_t vm_stk{};
    size_t vm_exe{};
    size_t vm_lib{};
    size_t vm_pte{};
    size_t vm_swap{};
};

using Metrics = uint64_t;
inline constexpr Metrics VM_PEAK{Metrics{1} << 0};
inline constexpr Metrics VM_SIZE{Metrics(1) << 1};
inline constexpr Metrics VM_LCK{Metrics(1) << 2};
inline constexpr Metrics VM_PIN{Metrics(1) << 3};
inline constexpr Metrics VM_HWM{Metrics(1) << 4};
inline constexpr Metrics VM_RSS{Metrics(1) << 5};
inline constexpr Metrics RSS_ANON{Metrics(1) << 6};
inline constexpr Metrics RSS_FILE{Metrics(1) << 7};
inline constexpr Metrics RSS_SHMEM{Metrics(1) << 8};
inline constexpr Metrics VM_DATA{Metrics(1) << 9};
inline constexpr Metrics VM_STK{Metrics(1) << 10};
inline constexpr Metrics VM_EXE{Metrics(1) << 11};
inline constexpr Metrics VM_LIB{Metrics(1) << 12};
inline constexpr Metrics VM_PTE{Metrics(1) << 13};
inline constexpr Metrics VM_SWAP{Metrics(1) << 14};

Info Get();
Info Get(Metrics metrics);
Info Get(int pid);
Info Get(int pid, Metrics metrics);
}
namespace lscpu {
struct Info {
std::string architecture;
size_t cpus{};
size_t threads_per_core{};
size_t cores_per_socket{};
size_t sockets{};
size_t numa_nodes{};
std::string model_name;
double cpu_mhz;
};

[[nodiscard]] Info Get();
}
}
