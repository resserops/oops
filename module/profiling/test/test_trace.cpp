#include <thread>

#include "gtest/gtest.h"

#define OOPS_TRACE_LEVEL OOPS_TRACE_LEVEL_DEBUG
#include "oops/once.h"
#include "oops/trace.h"

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
        TRACE_SCOPE(INFO);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        TRACE("step1");
        for (size_t i{0}; i < LOOP_N; ++i) {
            TRACE_SCOPE(VERBOSE);
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            TRACE("step2_1");
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            TRACE("step2_2");
        }
        TRACE("step2");
        for (size_t i{0}; i < LOOP_N; ++i) {
            TRACE_SCOPE(INFO);

            if (i % 3 == 0) {
                TRACE_SCOPE(VERBOSE);
                std::this_thread::sleep_for(std::chrono::milliseconds(10));

                [[maybe_unused]] int a = 10;

                TRACE("step3_1", MEM, [&](const Sample &sample) { std::cout << sample << " " << a << std::endl; });
            } else {
                TRACE_SCOPE(INFO);
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                TRACE("step3_2", MEM);
            }
        }
        TRACE("step3");
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    ParallelTraceStore::Get().GetRecordTable().Output(std::cout);
}

void Func() {
    TRACE_SCOPE(INFO);
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    TRACE("INFUNC");
}

TEST(ProfilingTrace, BAD) {
    ParallelTraceStore::Get().Clear();
    TRACE_SCOPE(INFO);
    Func();
    for (size_t i = 0; i < 1; ++i) {
        TRACE_SCOPE(INFO);
        Func();
        TRACE("FOR_DONE");
    }
    Func();
    TRACE("DONE");
    ParallelTraceStore::Get().GetRecordTable().Output(std::cout);
}

TEST(ProfilingTrace, InternalCost) {
    ParallelTraceStore::Get().Clear();
    constexpr size_t LOOP_N{10};
    TRACE_SCOPE(INFO);
    for (size_t i{0}; i < LOOP_N; ++i) {
        TRACE_SCOPE(INFO);
        microsleep_mono(100);
        TRACE("step1");
        TRACE("step2");
    }
    ParallelTraceStore::Get().GetRecordTable().Output(std::cout);
}

TEST(ProfilingTrace, Bad) {
    ParallelTraceStore::Get().Clear();
    TRACE_SCOPE(INFO);
    for (size_t i = 0; i < 2; ++i) {
        try {
            TRACE("step1");
        } catch (...) {
            EXPECT_EQ(i, 1);
            break;
        }
        EXPECT_EQ(i, 0);
    }
}

TEST(ProfilingTrace, Bad2) {
    ParallelTraceStore::Get().Clear();
    for (size_t i = 0; i < 10; ++i) {
        TRACE_SCOPE(INFO);
        if (i % 2 == 0) {
            TRACE("step_n0");
        } else {
            try {
                TRACE("step_n1");
            } catch (...) {
                EXPECT_EQ(i, 1);
                break;
            }
        }
    }
}

TEST(ProfilingTrace, Bad3) {
    ParallelTraceStore::Get().Clear();
    for (size_t i = 0; i < 10; ++i) {
        TRACE_SCOPE(INFO);
        if (i % 2 == 0) {
            try {
                TRACE("step1");
            } catch (...) {
                EXPECT_EQ(i, 2);
                break;
            }
        }
        EXPECT_LT(i, 2);
    }
}

TEST(ProfilingTrace, TraceBaseDemo) {
    ParallelTraceStore::Get().Clear();
    auto funcB = [] {
        TRACE_SCOPE(INFO);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        TRACE("FuncD");
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        TRACE("FuncE");
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        TRACE("FuncF");
    };
    TRACE_SCOPE(INFO);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    TRACE("FuncA", MEM);
    funcB();
    TRACE("FuncB", MEM);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    TRACE("FuncC", MEM);
    ParallelTraceStore::Get().GetRecordTable().Output(std::cout);
}

TEST(ProfilingTrace, Parallel) {
    ParallelTraceStore::Get().Clear();

    auto func = [] {
        TRACE_SCOPE(INFO);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        TRACE("step1");
        for (int i = 0; i < 100; ++i) {
            TRACE_SCOPE(INFO);
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            TRACE("in_loop");
        }
        TRACE("step2");
    };
    TRACE_SCOPE(INFO);
    std::vector<std::thread> threads;
    for (size_t i{0}; i < 8; ++i) {
        threads.emplace_back(func);
    }
    TRACE("Launched");
    for (auto &t : threads) {
        t.join();
    }
    TRACE("Joined");
    ParallelTraceStore::Get().GetRecordTable().Output(std::cout);
}
