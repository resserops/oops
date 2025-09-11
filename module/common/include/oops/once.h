#pragma once

#include <cstddef>

#define OOPS_ONLY(n) if (::oops::detail::Only<n>::F([]{}))
#ifndef ONLY
#define ONLY(n) OOPS_ONLY(n)
#endif

#define OOPS_ONCE() OOPS_ONLY(1)
#ifndef ONCE
#define ONCE() OOPS_ONCE()
#endif

#define OOPS_TWICE() OOPS_ONLY(2)
#ifndef TWICE
#define TWICE() OOPS_TWICE()
#endif

#define OOPS_EVERY(n) if (::oops::detail::Every<n>::F([]{}))
#ifndef EVERY
#define EVERY(n) OOPS_EVERY(n)
#endif

namespace oops {
namespace detail {
template <size_t N>
struct Only {
    template <typename Lambda>
    static bool F(Lambda &&) {
        static size_t count{0};
        if (count < N) {
            ++count;
            return true;
        }
        return false;
    }
};

template <>
struct Only<1> {
    template <typename Lambda>
    static bool F(Lambda &&) {
        static bool done{false};
        if (!done) {
            done = true;
            return true;
        }
        return false;
    }
};

template <>
struct Only<0> {
    template <typename Lambda>
    constexpr static bool F(Lambda &&) {
        return false;
    }
};

template <size_t N>
struct Every {
    static_assert(N > 0);
    template <typename Lambda>
    static bool F(Lambda &&) {
        static size_t count{0};
        if (count >= N) {
            count = 0;
        }
        return count++ == 0;
    }
};

template <>
struct Every<1> {
    template <typename Lambda>
    constexpr static bool F(Lambda &&) {
        return true;
    }
};
}
}
