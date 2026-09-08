#pragma once

#include <cstddef>
#include <iosfwd>
#include <string>

#include "oops/enum_bitset.h"

namespace oops {
namespace lscpu {
// clang-format off
enum class Field {
    ARCHITECTURE,     CPUS,             THREADS_PER_CORE, CORES_PER_SOCKET, SOCKETS,          NUMA_NODES,
    MODEL_NAME,       CPU_MHZ,          COUNT
};
// clang-format on
using FieldMask = EnumBitset<Field>;
using oops::operator|;

struct Info { // 仅支持部分字段
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
::std::ostream &operator<<(::std::ostream &os, const Info &info);
} // namespace lscpu
} // namespace oops
