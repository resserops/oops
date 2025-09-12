#include "oops/str.h"

#include <cassert>
#include <cstring>

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

namespace str {
bool StartsWith(std::string_view sv, std::string_view prefix) {
  if (prefix.empty()) {
    return true;
  }
  if (prefix.size() > sv.size()) {
    return false;
  }
  return sv.substr(0, prefix.size()) == prefix;
}

bool EndsWith(std::string_view sv, std::string_view suffix) {
  if (suffix.empty()) {
    return true;
  }
  if (suffix.size() > sv.size()) {
    return false;
  }
  return sv.substr(sv.size() - suffix.size()) == suffix;
}

std::string_view Strip(std::string_view sv, std::string_view chars) {
  if (sv.empty()) {
    return sv;
  }

  std::size_t pos{sv.find_first_not_of(chars)};
  if (pos == sv.npos) {
    return {};
  }

  return sv.substr(pos, sv.find_last_not_of(chars) - pos + 1);
}
} // namespace str
} // namespace oops
