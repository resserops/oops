#pragma once

#include <cstddef>
#include <sstream>
#include <string>
#include <string_view>

namespace oops {
constexpr std::string_view SPACE{" \t\n\r\v\f"};
std::string Repeat(const std::string &str, std::size_t n);
std::string SplitBack(const std::string &str, const std::string &delim);

template <typename Iter>
void Split(std::string_view s, std::string_view delim, bool skip_empty, Iter iter) {
    std::size_t begin{0}; // 每个分割字串的头部
    while (begin < s.size()) {
        std::size_t end{s.find(delim, begin)};
        if (end == std::string::npos) {
            break;
        }
        if (!skip_empty || end > begin) {
            // TODO(resserops): 支持基于lexical_cast的转换
            *(iter++) = s.substr(begin, end - begin);
        }
        begin = end + delim.size();
    }
    if (!skip_empty || s.size() > begin) {
        *(iter++) = s.substr(begin);
    }
}

template <typename Iter>
void Split(std::string_view s, std::string_view delim, Iter iter) {
    Split(s, delim, false, iter);
}

template <typename Iter>
void Split(std::string_view s, const char *delim, Iter iter) {
    Split(s, delim, false, iter);
}

template <typename Iter>
void SplitMultiDelim(std::string_view s, std::string_view delims, bool skip_empty, Iter iter) {
    std::size_t begin{0};
    while (begin < s.size()) {
        std::size_t end{s.find_first_of(delims, begin)};
        if (end == std::string::npos) {
            break;
        }
        if (!skip_empty || end > begin) {
            *(iter++) = s.substr(begin, end - begin);
        }
        begin = end + 1;
    }
    if (!skip_empty || s.size() > begin) {
        *(iter++) = s.substr(begin);
    }
}

template <typename Iter>
void SplitMultiDelim(std::string_view s, std::string_view delims, Iter iter) {
    SplitMultiDelim(s, delims, false, iter);
}

template <typename Iter>
void Split(std::string_view s, bool skip_empty, Iter iter) {
    return SplitMultiDelim(s, SPACE, skip_empty, iter);
}

template <typename Iter>
void Split(std::string_view s, Iter iter) {
    return SplitMultiDelim(s, SPACE, true, iter);
}

template <typename Container, typename... Args>
Container Split(Args &&...args) {
    Container container;
    Split(std::forward<Args>(args)..., std::back_inserter(container));
    return container;
}

template <typename Container, typename... Args>
Container SplitMultiDelim(Args &&...args) {
    Container container;
    SplitMultiDelim(std::forward<Args>(args)..., std::back_inserter(container));
    return container;
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
    for (std::size_t i{0}, j{0}; true; ++i, ++j) {
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

constexpr bool StartsWith(std::string_view s, std::string_view prefix) {
    if (prefix.empty()) {
        return true;
    }
    if (prefix.size() > s.size()) {
        return false;
    }
    return s.substr(0, prefix.size()) == prefix;
}

constexpr bool EndsWith(std::string_view s, std::string_view suffix) {
    if (suffix.empty()) {
        return true;
    }
    if (suffix.size() > s.size()) {
        return false;
    }
    return s.substr(s.size() - suffix.size()) == suffix;
}

constexpr std::string_view Strip(std::string_view s, std::string_view chars = SPACE) {
    if (s.empty()) {
        return s;
    }
    std::size_t pos{s.find_first_not_of(chars)};
    if (pos == std::string_view::npos) {
        return {};
    }
    return s.substr(pos, s.find_last_not_of(chars) - pos + 1);
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
