#pragma once

// 定义TRACE_SCOPE
#define OOPS_TRACE_SCOPE_NAMED(name) ::oops::TraceScope name{::oops::MakeTraceScope([]{})}

#if OOPS_ENABLE_TRACE
#define OOPS_TRACE_SCOPE() OOPS_TRACE_SCOPE_NAMED(__trace_scope##line)
#else
#define OOPS_TRACE_SCOPE()
#endif

#ifndef TRACE_SCOPE
#define TRACE_SCOPE() OOPS_TRACE_SCOPE()
#endif

// 定义TRACE
#if OOPS_ENABLE_TRACE
#define OOPS_TRACE(label) ::oops::Trace(#label, __FILE__, __LINE__, []{})
#else
#define OOPS_TRACE(label)
#endif

#ifndef TRACE
#define TRACE(label) OOPS_TRACE(label)
#endif

// 定义TRACE_OUTPUT
#if OOPS_ENABLE_TRACE
#define OOPS_TRACE_OUTPUT_IMPL(out, label) ::oops::TraceStat::Get().GetRecordTable(label).Output(out)
#else
#define OOPS_TRACE_OUTPUT_IMPL(out, label) 
#endif

#define OOPS_TRACE_OUTPUT(out, label) OOPS_TRACE_OUTPUT_IMPL(out, #label)

#ifndef TRACE_OUTPUT
#define TRACE_OUTPUT(out, label) OOPS_TRACE_OUTPUT(out, label)
#endif

#define OOPS_TRACE_PRINT() OOPS_TRACE_OUTPUT_IMPL(::std::cout, nullptr)

#ifndef TRACE_PRINT
#define TRACE_PRINT() OOPS_TRACE_PRINT()
#endif
