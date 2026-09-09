#pragma once
#include <cstdint>
#include <iosfwd>

#include <sys/types.h>

#include "oops/enum_bitset.h"

namespace oops {
namespace proc {
namespace task {
namespace status {
// clang-format off
enum class Field : uint8_t {
    VM_PEAK,   VM_SIZE,   VM_LCK,    VM_PIN,    VM_HWM,    VM_RSS,    RSS_ANON,  RSS_FILE,  RSS_SHMEM, VM_DATA,
    VM_STK,    VM_EXE,    VM_LIB,    VM_PTE,    VM_SWAP,   COUNT
};
// clang-format on
using FieldMask = EnumBitset<Field>;
using oops::operator|; // 支持Field和FieldMask或运算ADL

struct Info { // 仅支持部分字段
    std::size_t vm_peak{};
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
[[nodiscard]] Info Get(const FieldMask &field_mask);
[[nodiscard]] Info Get(std::istream &is);
[[nodiscard]] Info Get(std::istream &is, const FieldMask &field_mask);
[[nodiscard]] Info Get(pid_t pid);                                         // /proc/{pid}
[[nodiscard]] Info Get(pid_t pid, const FieldMask &field_mask);            // /proc/{pid}
[[nodiscard]] Info Get(pid_t pid, pid_t tid);                              // /proc/{pid}/task/{tid}
[[nodiscard]] Info Get(pid_t pid, pid_t tid, const FieldMask &field_mask); // /proc/{pid}/task/{tid}
std::ostream &operator<<(std::ostream &os, const Info &info);
} // namespace status
} // namespace task
} // namespace proc
} // namespace oops
