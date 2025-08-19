#pragma once

#include <cstddef>

#define OOPS_ONLY(n) if (::oops::impl::Only<n>([]{}))
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

#define OOPS_EVERY(n) if (::oops::impl::Every<n>([]{}))
#ifndef EVERY
#define EVERY(n) OOPS_EVERY(n)
#endif

namespace oops {
namespace impl {
template <size_t N, typename F>
bool Only(F &&) {
    static size_t count{0};
    if (count < N) {
        ++count;
        return true;
    }
    return false;
}

template <size_t N, typename F>
bool Every(F &&) {
    static size_t count{0};
    if (count >= N) {
        count = 0;
    }
    return count++ == 0;
}
}
}
