#include <thread>

#include "gtest/gtest.h"

#define OOPS_ENABLE_TRACE true
#include "oops/trace.h"
#include "oops/once.h"

using namespace oops;

#include <time.h>

void microsleep_mono(long microseconds) {
    struct timespec ts;
    ts.tv_sec = microseconds / 1000000;
    ts.tv_nsec = (microseconds % 1000000) * 1000;

    while (clock_nanosleep(CLOCK_MONOTONIC, 0, &ts, &ts) == EINTR) {
        continue;
    }
}

TEST(ProfilingTrace, TraceBase) {
    constexpr size_t LOOP_N{10};
    {
        TRACE_SCOPE();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        TRACE(step1);
        for (size_t i{0}; i < LOOP_N; ++i) {
            TRACE_SCOPE();
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            TRACE(step2-1);
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            TRACE(step2-2);
        }
        TRACE(step2);
        for (size_t i{0}; i < LOOP_N; ++i) {
            TRACE_SCOPE();
            if (i % 3 == 0) {
                TRACE_SCOPE();
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                TRACE(step3-1);
            } else {
                TRACE_SCOPE();
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                TRACE(step3-2);
            }
        }
        TRACE(step3);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    TRACE_PRINT();
}

void Func() {
    TRACE_SCOPE();
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    TRACE(INFUNC);
}

TEST(ProfilingTrace, BAD) {
    TraceStat::Get().Clear();
    TRACE_SCOPE();
    Func();
    for (size_t i = 0; i < 1; ++i) {
        TRACE_SCOPE();
        Func();
        TRACE(FOR_DONE);
    }
    Func();
    TRACE(DONE);
    TRACE_PRINT();
}


TEST(ProfilingTrace, InternalCost) {
    TraceStat::Get().Clear();
    constexpr size_t LOOP_N{10};
    int k{0};
    TRACE_SCOPE();
    for (size_t i{0}; i < LOOP_N; ++i) {
        TRACE_SCOPE();
        microsleep_mono(100);
        TRACE(step1);
        TRACE(step2);
    }
    TRACE_PRINT();
    std::cout << k << std::endl;
}
