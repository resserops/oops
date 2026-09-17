#pragma once
#include <cstdint>
#include <vector>

namespace oops {
namespace cpu_freq {
enum class Source : uint8_t { SYSFS, CPUINFO, NONE };

struct Info {
    struct Entry {
        std::size_t id{};
        double mhz{};
    };

    [[nodiscard]] double Average() const;
    [[nodiscard]] double Max() const;
    [[nodiscard]] double Min() const;

    Source source{Source::NONE};
    std::vector<Entry> cpus;
};

[[nodiscard]] Info Get();
} // namespace cpu_freq
} // namespace oops
