#include "oops/str.h"

#include <cstring>
#include <cassert>

namespace oops {
std::string StrRepeat(const std::string &str, size_t n) {
    if (n == 0 || str.empty()) {
        return {};
    }
    std::string ret;
    ret.reserve(n * str.size());
    for (size_t i{0}; i < n; ++i) {
        ret.append(str);
    }
    return ret;
}

std::string operator*(const std::string &str, size_t n) {
    return StrRepeat(str, n);
}

std::string operator*(size_t n, const std::string &str) {
    return StrRepeat(str, n);
}

std::string StrSplitBack(const std::string &str, const std::string &delim) {
    return str.substr(str.rfind(delim) + delim.size());
}
}
