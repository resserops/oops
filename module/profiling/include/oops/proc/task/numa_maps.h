#pragma once
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace oops {
namespace proc {
namespace task {
namespace numa_maps {
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
} // namespace task
} // namespace proc
} // namespace oops
