#include "oops/proc/task/maps.h"

#include <fstream>
#include <istream>
#include <ostream>
#include <utility>

#include "fmt/format.h"

namespace oops {
namespace proc {
namespace task {
namespace maps {
Info Get() {
    std::ifstream ifs("/proc/self/maps");
    return Get(ifs);
}

Info Get(std::istream &is) {
    Info info;
    while (auto res{ParseVma(is)}) {
        info.vma_table.push_back(std::move(res.vma));
    }
    return info;
}

Info Get(pid_t pid) {
    std::ifstream ifs(fmt::format("/proc/{}/maps", pid));
    return Get(ifs);
}

Info Get(pid_t pid, pid_t tid) {
    std::ifstream ifs(fmt::format("/proc/{}/task/{}/maps", pid, tid));
    return Get(ifs);
}

std::ostream &operator<<(std::ostream &os, const Info &info) {
    for (const auto &vma : info.vma_table) {
        FormatVma(os, vma);
    }
    return os;
}
} // namespace maps
} // namespace task
} // namespace proc
} // namespace oops
