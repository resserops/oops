#include "oops/str.h"

namespace oops {
std::string RepeatStr(const std::string &str, size_t n) {
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
    return RepeatStr(str, n);
}

std::string operator*(size_t n, const std::string &str) {
    return RepeatStr(str, n);
}
}
