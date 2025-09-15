#pragma once

// 定义TRACE_SCOPE
#if OOPS_ENABLE_TRACE
#define OOPS_TRACE_SCOPE()                                                                                             \
    auto oops_trace_scope__ {                                                                                          \
        ::oops::MakeTraceScope([] {})                                                                                  \
    }
#else
#define OOPS_TRACE_SCOPE() (void)0
#endif

#ifndef TRACE_SCOPE
#define TRACE_SCOPE() OOPS_TRACE_SCOPE()
#endif

// 定义TRACE
#if OOPS_ENABLE_TRACE
#define OOPS_TRACE(label, ...)                                                                                         \
    oops_trace_scope__.Trace([] {}, #label, __FILE__, __LINE__, ::oops::detail::ParseTraceVaArgs(__VA_ARGS__))
#else
#define OOPS_TRACE(label, ...) (void)0
#endif

#ifndef TRACE
#define TRACE(label, ...) OOPS_TRACE(label, __VA_ARGS__)
#endif
