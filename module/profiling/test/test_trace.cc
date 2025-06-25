#include <thread>

#include "gtest/gtest.h"

#define OOPS_ENABLE_TRACE true
#include "oops/trace.h"

using namespace oops;

TEST(ProfilingTrace, TraceBase) {
    constexpr size_t LOOP_N{100};
    {
        TRACE_SCOPE();
        std::this_thread::sleep_for(std::chrono::microseconds(1));
        TRACE(step1);
        for (size_t i{0}; i < LOOP_N; ++i) {
            TRACE_SCOPE();
            std::this_thread::sleep_for(std::chrono::microseconds(1));
            TRACE(step2-1);
            std::this_thread::sleep_for(std::chrono::microseconds(1));
            TRACE(step2-2);
        }
        TRACE(step2);
        for (size_t i{0}; i < LOOP_N; ++i) {
            TRACE_SCOPE();
            if (i % 3 == 0) {
                TRACE_SCOPE();
                std::this_thread::sleep_for(std::chrono::microseconds(1));
                TRACE(step3-1);
            } else {
                TRACE_SCOPE();
                std::this_thread::sleep_for(std::chrono::microseconds(1));
                TRACE(step3-2);
            }
        }
        TRACE(step3);
    }
    TRACE_PRINT();
}
