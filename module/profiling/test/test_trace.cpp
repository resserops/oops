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
    TraceConfig::Get().SetAnonymous();
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

                int a = 10;

                TRACE(step3-1, MEM, [&](const Sample &sample){ std::cout << sample << " " << a << std::endl; });
            } else {
                TRACE_SCOPE();
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                TRACE(step3-2, MEM);
            }
        }
        TRACE(step3);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    RecordStore::Get().GetRecordTable().Output(std::cout);
}

void Func() {
    TRACE_SCOPE();
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    TRACE(INFUNC);
}

TEST(ProfilingTrace, BAD) {
    RecordStore::Get().Clear();
    TRACE_SCOPE();
    Func();
    for (size_t i = 0; i < 1; ++i) {
        TRACE_SCOPE();
        Func();
        TRACE(FOR_DONE);
    }
    Func();
    TRACE(DONE);
    RecordStore::Get().GetRecordTable().Output(std::cout);
}

TEST(ProfilingTrace, InternalCost) {
    RecordStore::Get().Clear();
    constexpr size_t LOOP_N{10};
    TRACE_SCOPE();
    for (size_t i{0}; i < LOOP_N; ++i) {
        TRACE_SCOPE();
        microsleep_mono(100);
        TRACE(step1);
        TRACE(step2);
    }
    RecordStore::Get().GetRecordTable().Output(std::cout);
}

TEST(ProfilingTrace, Bad) {
    RecordStore::Get().Clear();
    TRACE_SCOPE();
    for (size_t i=0;i<2;++i) {
        try {
            TRACE(step1);
        } catch (...) {
            EXPECT_EQ(i, 1);
            break;
        }
        EXPECT_EQ(i, 0);
    }
}

TEST(ProfilingTrace, Bad2) {
    RecordStore::Get().Clear();
    for (size_t i=0;i<10;++i) {
        TRACE_SCOPE();
        if (i % 2 == 0) {
            TRACE(step_n0);
        } else {
            try {
                TRACE(step_n1);
            } catch (...) {
                EXPECT_EQ(i, 1);
                break;
            }
        }
    }
}

TEST(ProfilingTrace, Bad3) {
    RecordStore::Get().Clear();
    for (size_t i=0;i<10;++i) {
        TRACE_SCOPE();
        if (i % 2 == 0) {
            try {
                TRACE(step1);
            } catch (...) {
                EXPECT_EQ(i, 2);
                break;
            }
        }
        EXPECT_LT(i, 2);
    }
}