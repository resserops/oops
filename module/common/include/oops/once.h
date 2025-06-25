#pragma once

#include <cstddef>

#define OOPS_ONLY(n) if (::oops::OnlyImpl<n>([]{}))
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

namespace oops {
template <size_t N, typename F>
bool OnlyImpl(F &&f) {
    static size_t count{N};
    if (count > 0) {
        --count;
        return true;
    }
    return false;
}
}
