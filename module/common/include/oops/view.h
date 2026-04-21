#pragma once

#include <cassert>
#include <cstdint>
#include <iterator>
#include <limits>
#include <utility>

namespace oops {
namespace view { // C++标准库设计语境下，view是非持有、惰性求值的轻量级range
template <typename T>
class ReverseView {
public:
    explicit ReverseView(T &&t) : t_{std::forward<T>(t)} {}

    auto begin() { return ::std::rbegin(t_); }
    auto end() { return ::std::rend(t_); }

    auto begin() const { return ::std::crbegin(t_); }
    auto end() const { return ::std::crend(t_); }

    auto cbegin() const { return ::std::crbegin(t_); }
    auto cend() const { return ::std::crend(t_); }

    auto rbegin() { return ::std::begin(t_); }
    auto rend() { return ::std::end(t_); }

    auto rbegin() const { return ::std::cbegin(t_); }
    auto rend() const { return ::std::cend(t_); }

    auto crbegin() const { return ::std::cbegin(t_); }
    auto crend() const { return ::std::cend(t_); }

private:
    T t_;
};

template <typename T>
auto Reverse(T &&t) {
    return ReverseView<T>(std::forward<T>(t));
}

template <typename T>
struct ArithmeticOverflowResult {
    constexpr explicit operator bool() const noexcept { return overflow; }
    T value{};
    bool overflow{};
};

template <typename T>
[[nodiscard]] constexpr auto AddOverflowGeneric(T a, T b) noexcept {
    static_assert(std::is_integral_v<T>);
    using UnsignedT = std::make_unsigned_t<T>;

    ArithmeticOverflowResult<T> res;
    res.value = static_cast<T>(static_cast<UnsignedT>(a) + static_cast<UnsignedT>(b));

    if constexpr (std::is_unsigned_v<T>) {
        res.overflow = res.value < a;
    } else {
        res.overflow = ((a ^ b) >= 0) && ((a ^ res.value) < 0);
    }
    return res;
}

template <typename T>
[[nodiscard]] constexpr auto AddOverflow(T a, T b) noexcept {
#if defined(__GNUC__) || defined(__clang__)
    ArithmeticOverflowResult<T> res;
    res.overflow = __builtin_add_overflow(a, b, &res.value);
    return res;
#else
    return AddOverflowGeneric(a, b);
#endif
}

template <typename T, typename Stop = T>
class RangeView {
    static_assert(std::is_integral_v<T>);
    static_assert(std::is_same_v<T, Stop> || !std::is_integral_v<Stop>);

public:
    class Iterator;
    constexpr RangeView(Stop stop) : stop_{stop} {}
    constexpr RangeView(T start, Stop stop, T step = 1) : start_{start}, stop_{stop}, step_{step} {
        if (step == 0) {
            throw std::invalid_argument("range() arg 3 must not be zero");
        }
    }

    constexpr Iterator begin() const { return Iterator(start_, step_); }
    constexpr Stop end() const { return stop_; }

private:
    T start_{0};
    Stop stop_;
    T step_{1};
};

template <typename T, typename Stop>
class RangeView<T, Stop>::Iterator {
public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = T;
    using difference_type = std::ptrdiff_t;
    using pointer = const T *;
    using reference = const T &;

    constexpr explicit Iterator(T t, T step) : t_{t}, step_{step} {}
    constexpr T operator*() const noexcept { return t_; }

    constexpr Iterator &operator++() noexcept {
        auto res{AddOverflow(t_, step_)};
        t_ = res.value;
        overflow_ = res.overflow;
        return *this;
    }

    constexpr Iterator operator++(int) noexcept {
        auto iter{*this};
        ++(*this);
        return iter;
    }

    bool Overflow() const { return overflow_; }
    T Step() const { return step_; }

    constexpr bool operator==(Stop stop) const {
        if constexpr (std::is_integral_v<Stop>) {
            if (step_ > 0) {
                return stop <= t_;
            }
            return stop >= t_;
        } else {
            return stop == (*this);
        }
    }
    constexpr bool operator!=(Stop stop) const { return !operator==(stop); }

protected:
    T t_, step_;
    bool overflow_{false};
};

template <typename T, typename Stop>
constexpr auto Range(T start, Stop stop, T step = 1) {
    if constexpr (std::is_integral_v<Stop>) {
        return RangeView<T>(start, stop, step);
    } else {
        return RangeView<T, Stop>(start, stop, step);
    }
}

template <typename T, typename Stop, std::enable_if_t<!std::is_integral_v<Stop>, int> = 0>
constexpr auto Range(Stop stop) {
    return RangeView<T, Stop>(stop);
}

template <typename T>
constexpr auto Range(T stop) {
    return RangeView<T>(stop);
}

struct RangeOverflow {
    template <typename Iter>
    constexpr bool operator==(const Iter &iter) const noexcept {
        return iter.Overflow();
    }
};
constexpr RangeOverflow RANGE_OVERFLOW;

template <typename T>
constexpr auto IntegerSet{Range(std::numeric_limits<T>::min(), RANGE_OVERFLOW)};
} // namespace view
} // namespace oops
