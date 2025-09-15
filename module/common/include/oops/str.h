#pragma once

#include <cstddef>
#include <string>
#include <string_view>

namespace oops {
std::string StrRepeat(const std::string &str, size_t n);
std::string operator*(const std::string &str, size_t n);
std::string operator*(size_t n, const std::string &str);

std::string StrSplitBack(const std::string &str, const std::string &delim);

template <typename Iter>
void StrSplitToIter(const std::string &str, Iter iter);
template <typename Iter>
void StrSplitToIter(const std::string &str, const std::string &delim, Iter iter);
template <typename Iter>
void StrSplitToIter(const std::string &str, const std::string &delim, bool skip_empty, Iter iter);

template <typename Iter>
void StrSplitToIterMultiDelim(const std::string &str, const std::string &delims, bool skip_empty, Iter iter);
template <typename Iter>
void StrSplitToIterMultiDelim(const std::string &str, const std::string &delims, Iter iter);

template <typename T>
std::string ToStr(const T &t);

namespace str {
constexpr std::string_view WHITE_SPACE{" \t\n\r\v\f"};
[[nodiscard]] bool StartsWith(std::string_view sv, std::string_view prefix);
[[nodiscard]] bool EndsWith(std::string_view sv, std::string_view suffix);
[[nodiscard]] std::string_view Strip(std::string_view sv, std::string_view chars = WHITE_SPACE);

template <typename T>
T FromStr(const std::string_view sv);
} // namespace str
} // namespace oops

#include "oops/str.tpp"
