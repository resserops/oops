#pragma once

#include <cstddef>
#include <string>

namespace oops {
std::string RepeatStr(const std::string &str, size_t n);
std::string operator*(const std::string &str, size_t n);
std::string operator*(size_t n, const std::string &str);
}
