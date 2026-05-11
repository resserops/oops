#include "oops/str.h"

#include <cassert>
#include <cstring>

namespace oops {
std::string Repeat(const std::string &str, std::size_t n) {
    if (n == 0 || str.empty()) {
        return {};
    }
    std::string res;
    res.reserve(n * str.size());
    for (std::size_t i{0}; i < n; ++i) {
        res.append(str);
    }
    return res;
}

std::string SplitBack(const std::string &str, const std::string &delim) {
    return str.substr(str.rfind(delim) + delim.size());
}

std::string ToLower(std::string_view s) noexcept {
    std::string res;
    res.reserve(s.size());
    for (char ch : s) {
        res.push_back(ToLower(ch));
    }
    return res;
}
} // namespace oops
