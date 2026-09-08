#pragma once

#include <cstdint>
#include <iosfwd>
#include <string>
#include <vector>

#include "oops/enum_bitset.h"
#include "oops/unit.h"

namespace oops {
namespace proc {
namespace cpuinfo {
// 每个processor一个记录，字段集与本机/proc/cpuinfo打印一致，Field枚举顺序与Processor成员顺序一致
// clang-format off
enum class Field : uint8_t {
    PROCESSOR,        VENDOR_ID,        CPU_FAMILY,       MODEL,            MODEL_NAME,       STEPPING,
    MICROCODE,        CPU_MHZ,          CACHE_SIZE,       PHYSICAL_ID,      SIBLINGS,         CORE_ID,
    CPU_CORES,        APICID,           INITIAL_APICID,   FPU,              FPU_EXCEPTION,    CPUID_LEVEL,
    WP,               FLAGS,            BUGS,             BOGOMIPS,         CLFLUSH_SIZE,     CACHE_ALIGNMENT,
    ADDRESS_SIZES,    POWER_MANAGEMENT, COUNT
};
// clang-format on
using FieldMask = EnumBitset<Field>;
using oops::operator|; // 支持Field和FieldMask或运算ADL

struct Entry {
    // 标识
    std::size_t processor{};
    std::string vendor_id{};
    std::size_t cpu_family{};
    std::size_t model{};
    std::string model_name{};
    std::size_t stepping{};
    std::size_t microcode{}; // 十六进制形式，如0x1

    // 规格
    double cpu_mhz{};
    KiBs<std::size_t> cache_size{};

    // 拓扑
    std::size_t physical_id{};
    std::size_t siblings{};
    std::size_t core_id{};
    std::size_t cpu_cores{};
    std::size_t apicid{};
    std::size_t initial_apicid{};

    // 特性
    bool fpu{};
    bool fpu_exception{};
    std::size_t cpuid_level{};
    bool wp{};
    std::vector<std::string> flags{};
    std::vector<std::string> bugs{};

    // 其它
    double bogomips{};
    std::size_t clflush_size{};
    std::size_t cache_alignment{};
    std::string address_sizes{};
    std::vector<std::string> power_management{};

    FieldMask parsed;
};

struct Info {
    std::vector<Entry> table;
};

[[nodiscard]] Info Get();
[[nodiscard]] Info Get(const FieldMask &field_mask);
[[nodiscard]] Info Get(std::istream &is);
[[nodiscard]] Info Get(std::istream &is, const FieldMask &field_mask);
std::ostream &operator<<(std::ostream &os, const Info &info);
} // namespace cpuinfo
} // namespace proc
} // namespace oops
