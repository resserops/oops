#pragma once

#include <cstddef>
#include <string>
#include <string_view>

namespace oops {
std::string StrRepeat(const std::string &str, size_t n);
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
constexpr std::string_view SPACE{" \t\n\r\v\f"};

constexpr bool IsUpper(char c) noexcept;
constexpr bool IsLower(char c) noexcept;
constexpr bool IsAlpha(char c) noexcept;
constexpr bool IsDigit(char c) noexcept;
constexpr bool IsAlnum(char c) noexcept;
constexpr bool IsBlank(char c) noexcept;
constexpr char ToUpper(char c) noexcept;
constexpr char ToLower(char c) noexcept;

constexpr bool Equal(std::string_view lhs, std::string_view rhs) noexcept;
template <typename CharEqual>
constexpr bool Equal(std::string_view lhs, std::string_view rhs, CharEqual &&char_equal) noexcept;
template <typename CharEqual, typename Filter>
constexpr bool Equal(std::string_view lhs, std::string_view rhs, CharEqual &&char_equal, Filter &&filter) noexcept;

bool StartsWith(std::string_view sv, std::string_view prefix);
bool EndsWith(std::string_view sv, std::string_view suffix);
std::string_view Strip(std::string_view sv, std::string_view chars = SPACE);

template <typename T>
T FromStr(const std::string_view sv);
} // namespace str
} // namespace oops

#include "oops/str.tpp"
