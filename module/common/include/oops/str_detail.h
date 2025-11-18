#pragma once

#include <cstddef>
#include <iostream>
#include <sstream>
#include <string>

namespace oops {
template <typename Iter>
void Split(const std::string &str, Iter iter) {
    constexpr const char SPACE[]{" \t\n\r\v\f"};
    return SplitMultiDelim(str, SPACE, true, iter);
}

template <typename Iter>
void Split(const std::string &str, bool skip_empty, Iter iter) {
    std::cout << "Do" << std::endl;
    constexpr const char SPACE[]{" \t\n\r\v\f"};
    return SplitMultiDelim(str, SPACE, skip_empty, iter);
}

template <typename Iter>
void Split(const std::string &str, const char *delim, Iter iter) {
    Split(str, delim, false, iter);
}

template <typename Iter>
void Split(const std::string &str, const std::string &delim, Iter iter) {
    Split(str, delim, false, iter);
}

template <typename Iter>
void Split(const std::string &str, const std::string &delim, bool skip_empty, Iter iter) {
    size_t begin{0}; // 每个分割字串的头部
    while (begin < str.size()) {
        size_t end{str.find(delim, begin)};
        if (end == str.npos) {
            break;
        }
        if (!skip_empty || end > begin) {
            *(iter++) = str.substr(begin, end - begin);
        }
        begin = end + delim.size();
    }
    if (!skip_empty || str.size() > begin) {
        *(iter++) = str.substr(begin);
    }
}

template <typename Iter>
void SplitMultiDelim(const std::string &str, const std::string &delims, Iter iter) {
    SplitMultiDelim(str, delims, false, iter);
}

template <typename Iter>
void SplitMultiDelim(const std::string &str, const std::string &delims, bool skip_empty, Iter iter) {
    size_t begin{0};
    while (begin < str.size()) {
        size_t end{str.find_first_of(delims, begin)};
        if (end == str.npos) {
            break;
        }
        if (!skip_empty || end > begin) {
            *(iter++) = str.substr(begin, end - begin);
        }
        begin = end + 1;
    }
    if (!skip_empty || str.size() > begin) {
        *(iter++) = str.substr(begin);
    }
}

constexpr bool IsUpper(char c) noexcept { return 'A' <= c && c <= 'Z'; }
constexpr bool IsLower(char c) noexcept { return 'a' <= c && c <= 'z'; }
constexpr bool IsAlpha(char c) noexcept { return IsUpper(c) || IsLower(c); }
constexpr bool IsDigit(char c) noexcept { return '0' <= c && c <= '9'; }
constexpr bool IsAlnum(char c) noexcept { return IsAlpha(c) || IsDigit(c); }
constexpr bool IsSpace(char c) noexcept {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\v' || c == '\f';
}

constexpr char ToUpper(char c) noexcept { return IsLower(c) ? (c - 'a' + 'A') : c; }
constexpr char ToLower(char c) noexcept { return IsUpper(c) ? (c - 'A' + 'a') : c; }

constexpr bool Equal(std::string_view lhs, std::string_view rhs) noexcept { return lhs == rhs; }

template <typename CharEqual>
constexpr bool Equal(std::string_view lhs, std::string_view rhs, CharEqual &&char_equal) noexcept {
    if (lhs.size() != rhs.size()) {
        return false;
    }
    for (std::size_t i{0}; i < lhs.size(); ++i) {
        if (!char_equal(lhs[i], rhs[i])) {
            return false;
        }
    }
    return true;
}

template <typename CharEqual, typename Filter>
constexpr bool Equal(std::string_view lhs, std::string_view rhs, CharEqual &&char_equal, Filter &&filter) noexcept {
    for (size_t i{0}, j{0}; true; ++i, ++j) {
        while (i < lhs.size() && !filter(lhs[i])) {
            ++i;
        }
        while (j < rhs.size() && !filter(rhs[j])) {
            ++j;
        }
        if (i == lhs.size() && j == rhs.size()) {
            return true;
        }
        if (i == lhs.size() || j == rhs.size()) {
            return false;
        }
        if (!char_equal(lhs[i], rhs[j])) {
            return false;
        }
    }
}

template <typename T>
::std::string ToStr(const T &t) {
    ::std::ostringstream oss;
    oss << t;
    return oss.str();
}

template <typename T>
T FromStr(const ::std::string_view sv) {
    ::std::istringstream iss{::std::string{sv}}; // TODO(resserops): 优化自定义stream
    T t{};
    iss >> t;
    if (iss.fail()) {
        throw ::std::invalid_argument{""}; // TODO(resserops): 填写详细错误信息
    }
    return t;
}

template <>
inline ::std::string FromStr<::std::string>(const ::std::string_view sv) {
    return ::std::string{sv};
}
} // namespace oops
