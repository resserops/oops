#include "oops/str.h"

#include <cassert>
#include <cstring>

namespace oops {
std::string Repeat(const std::string &str, std::size_t n) {
    if (n == 0 || str.empty()) {
        return {};
    }
    std::string ret;
    ret.reserve(n * str.size());
    for (std::size_t i{0}; i < n; ++i) {
        ret.append(str);
    }
    return ret;
}

std::string SplitBack(const std::string &str, const std::string &delim) {
    return str.substr(str.rfind(delim) + delim.size());
}
} // namespace oops
