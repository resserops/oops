#pragma once

#include <cstddef>
#include <string>
#include <string_view>

namespace oops {
constexpr std::string_view SPACE{" \t\n\r\v\f"};
std::string Repeat(const std::string &str, size_t n);
std::string SplitBack(const std::string &str, const std::string &delim);

template <typename Iter>
void Split(const std::string &str, Iter iter);
template <typename Iter>
void Split(const std::string &str, bool skip_empty, Iter iter);
template <typename Iter>
void Split(const std::string &str, const char *delim, Iter iter);
template <typename Iter>
void Split(const std::string &str, const std::string &delim, Iter iter);
template <typename Iter>
void Split(const std::string &str, const std::string &delim, bool skip_empty, Iter iter);
template <typename Iter>
void SplitMultiDelim(const std::string &str, const std::string &delims, Iter iter);
template <typename Iter>
void SplitMultiDelim(const std::string &str, const std::string &delims, bool skip_empty, Iter iter);

template <typename Container>
Container Split(const std::string &str);
template <typename Container>
Container Split(const std::string &str, bool skip_empty);
template <typename Container>
Container Split(const std::string &str, const char *delim);
template <typename Container>
Container Split(const std::string &str, const std::string &delim);
template <typename Container>
Container Split(const std::string &str, const std::string &delim, bool skip_empty);
template <typename Container>
Container SplitMultiDelim(const std::string &str, const std::string &delims);
template <typename Container>
Container SplitMultiDelim(const std::string &str, const std::string &delims, bool skip_empty);

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
std::string ToStr(const T &t);
template <typename T>
T FromStr(const std::string_view sv);
} // namespace oops
#include "oops/str_impl.h"
