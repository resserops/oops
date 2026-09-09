#pragma once
#include <iosfwd>
#include <vector>

#include <sys/types.h> // 提供pid_t

#include "oops/proc/task/vma.h"

namespace oops {
namespace proc {
namespace task {
namespace maps {
struct Info {
    std::vector<Vma> vma_table;
};

[[nodiscard]] Info Get();
[[nodiscard]] Info Get(std::istream &is);
[[nodiscard]] Info Get(pid_t pid);            // /proc/{pid}
[[nodiscard]] Info Get(pid_t pid, pid_t tid); // /proc/{pid}/task/{tid}
std::ostream &operator<<(std::ostream &os, const Info &info);
} // namespace maps
} // namespace task
} // namespace proc
} // namespace oops
