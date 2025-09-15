#pragma once

#include <cstddef>
#include <functional>

namespace oops {
struct Sample;

namespace detail {
// TRACE宏可变参数解析
constexpr ::std::size_t ParseTraceVaArgs() { return 0; }
constexpr ::std::size_t ParseTraceVaArgs(::std::size_t mask) { return mask; }

using SampleHandler = ::std::function<void(const Sample &)>;
struct TraceVaArgs {
    ::std::size_t mask;
    const SampleHandler &sample_handler;
};

inline TraceVaArgs ParseTraceVaArgs(::std::size_t mask, const SampleHandler &sample_handler) {
    return {mask, sample_handler};
}
} // namespace detail
} // namespace oops
