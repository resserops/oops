#pragma once

#include <cstddef>
#include <sstream>
#include <string>
#include <string_view>

namespace oops {
constexpr std::string_view SPACE{" \t\n\r\v\f"};

// 字符操作
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

// 字符串读操作
std::string SplitBack(const std::string &str, const std::string &delim); // SplitView实现逆序迭代器后合并到Split中

class SplitView {
public:
    class Iterator {
    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = std::string_view;
        using difference_type = std::ptrdiff_t;
        using pointer = const std::string_view *;
        using reference = const std::string_view &;

        Iterator() = default;
        Iterator(std::string_view s, std::string_view delim) : s_{s}, delim_{delim} { NextToken(); }
        Iterator(std::string_view s, std::string_view delim, bool any_of_delims, bool skip_empty)
            : s_{s}, delim_{delim}, any_of_delims_{any_of_delims}, skip_empty_{skip_empty} {
            NextToken();
        }

        std::string_view operator*() const noexcept { return token_; }
        Iterator &operator++() {
            NextToken();
            return *this;
        }

        bool operator==(const Iterator &other) const {
            // 短路和迭代器的比较
            if ((s_.data() == nullptr) ^ (other.s_.data() == nullptr)) {
                return false; // 一个迭代器是尾迭代器，另一个不是
            }
            if (s_.data() == nullptr) {
                return true; // 均是尾迭代器
            }
            // 非尾迭代器的严格匹配
            return (s_.data() == other.s_.data()) && (s_.size() == other.s_.size()) &&
                   (token_.data() == other.token_.data()) && (token_.size() == other.token_.size()) &&
                   (delim_ == other.delim_) && (any_of_delims_ == other.any_of_delims_) &&
                   (skip_empty_ == other.skip_empty_);
        }
        bool operator!=(const Iterator &other) const { return !operator==(other); }

    private:
        void NextToken();

        std::string_view s_;
        std::string_view token_;
        const std::string_view delim_;
        const bool any_of_delims_{};
        const bool skip_empty_{};
    };

    using value_type = std::string_view;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using const_iterator = Iterator;
    using iterator = Iterator;

    SplitView(std::string_view s) : s_{s}, delim_{SPACE}, any_of_delims_{true}, skip_empty_{true} {}
    SplitView(std::string_view s, char delim) : s_{s}, delim_(&c_, 1), c_{delim} {}
    SplitView(std::string_view s, std::string_view delim) : s_{s}, delim_{delim} {}

    Iterator begin() const { return Iterator(s_, delim_, any_of_delims_, skip_empty_); }
    Iterator end() const { return Iterator(); }

    std::string_view Front() const { return *begin(); }
    bool Empty() const { return begin() == end(); }

    SplitView &AnyOfDelims(bool b = true) {
        any_of_delims_ = b;
        return *this;
    }

    SplitView &SkipEmpty(bool b = true) {
        skip_empty_ = b;
        return *this;
    }

    template <typename T>
    T To() const {
        return T(begin(), end());
    }

    template <template <typename...> typename TT>
    auto To() const {
        return To<TT<std::string_view>>();
    }

private:
    const std::string_view s_;
    const std::string_view delim_;
    bool any_of_delims_{};
    bool skip_empty_{};
    const char c_{}; // 支持单字符delim
};

inline SplitView Split(std::string_view s) { return SplitView(s); }
inline SplitView Split(std::string_view s, char delim) { return SplitView(s, delim); }
inline SplitView Split(std::string_view s, std::string_view delim) { return SplitView(s, delim); }

constexpr std::string_view StripLeft(std::string_view s, std::string_view chars = SPACE) {
    std::size_t pos{s.find_first_not_of(chars)};
    if (pos == std::string_view::npos) {
        pos = s.size();
    }
    s.remove_prefix(pos);
    return s;
}

constexpr std::string_view StripRight(std::string_view s, std::string_view chars = SPACE) {
    std::size_t pos{s.find_last_not_of(chars) + 1};
    if (pos == std::string_view::npos) {
        pos = s.size();
    }
    s.remove_suffix(s.size() - pos);
    return s;
}

constexpr std::string_view Strip(std::string_view s, std::string_view chars = SPACE) {
    return StripLeft(StripRight(s, chars), chars);
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
    for (std::size_t i{0}, j{0};; ++i, ++j) {
        while ((i < lhs.size()) && (!filter(lhs[i]))) {
            ++i;
        }
        while ((j < rhs.size()) && (!filter(rhs[j]))) {
            ++j;
        }
        if ((i == lhs.size()) ^ (j == rhs.size())) {
            return false; // 一个查找到末尾，另一个没有
        }
        if (i == lhs.size()) {
            return true;
        }
        if (!char_equal(lhs[i], rhs[j])) {
            return false;
        }
    }
}

// 字符串写操作
std::string ToLower(std::string_view s) noexcept;
std::string ToUpper(std::string_view s) noexcept;

std::string Repeat(const std::string &str, std::size_t n);
std::string Elide(std::string_view s, std::size_t n);

// 后续使用lexcial_cast替代
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
