#pragma once

#include <iterator>
#include <limits>
#include <utility>

namespace oops {
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
    return ReverseView<T>{std::forward<T>(t)};
}

struct UnreachableSentinel {
    template <typename T>
    bool operator==(const T &t) const noexcept {
        return false;
    }
};

template <typename T, typename Bound = UnreachableSentinel>
class IotaView {
public:
    class Iter {
    public:
        constexpr explicit Iter(T t) : t_{t} {}
        constexpr T operator*() const { return t_; }
        constexpr bool operator!=(const Bound &bound) const { return t_ != bound; }
        constexpr Iter &operator++() {
            ++t_;
            return *this;
        }

    private:
        T t_;
    };

    constexpr explicit IotaView(T t) : t_{t} {}
    constexpr IotaView(T t, Bound bound) : t_{t}, bound_{bound} {}

    constexpr Iter begin() const { return Iter(t_); }
    constexpr Bound end() const { return bound_; }

private:
    T t_;
    Bound bound_;
};

template <typename T, typename Bound>
constexpr auto Iota(T &&t, Bound &&bound) {
    return IotaView<T, Bound>(t, bound);
}

template <typename T>
constexpr auto Iota(T &&t) {
    return IotaView<T>(t);
}
} // namespace oops
