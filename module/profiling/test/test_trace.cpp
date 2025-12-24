#include <thread>

#include "gtest/gtest.h"

#define OOPS_TRACE_LEVEL OOPS_TRACE_LEVEL_DEBUG
#include "oops/once.h"
#include "oops/trace.h"

using namespace oops;

#include <time.h>

// 高效 busy-wait：目标是消耗 exactly 'us' 微秒的 CPU 时间
void cpu_burn_us(uint64_t us) {
    if (us == 0)
        return;

    struct timespec start, now;
    clock_gettime(CLOCK_MONOTONIC, &start);

    uint64_t end_ns = (uint64_t)start.tv_sec * 1000000000ULL + (uint64_t)start.tv_nsec + us * 1000ULL; // us → ns

    // 忙等待循环（不调用任何可能 sleep 的函数）
    do {
        // 关键：只做轻量级操作，避免 cache miss / 分支预测失败
        __asm__ volatile("");                 // 防止编译器优化掉空循环
        clock_gettime(CLOCK_MONOTONIC, &now); // 可接受少量开销（<1%）
    } while ((uint64_t)now.tv_sec * 1000000000ULL + (uint64_t)now.tv_nsec < end_ns);
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
    OOPS_TRACE_STORE::Get().GetRecordTable().Output(std::cout);
}

void Func() {
    TRACE_SCOPE(INFO);
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    TRACE("INFUNC");
}

TEST(ProfilingTrace, BAD) {
    OOPS_TRACE_STORE::Get().Clear();
    TRACE_SCOPE(INFO);
    Func();
    for (size_t i = 0; i < 1; ++i) {
        TRACE_SCOPE(INFO);
        Func();
        TRACE("FOR_DONE");
    }
    Func();
    TRACE("DONE");
    OOPS_TRACE_STORE::Get().GetRecordTable().Output(std::cout);
}

TEST(ProfilingTrace, InternalCost) {
    OOPS_TRACE_STORE::Get().Clear();
    constexpr size_t LOOP_N{10};
    TRACE_SCOPE(INFO);
    for (size_t i{0}; i < LOOP_N; ++i) {
        TRACE_SCOPE(INFO);
        cpu_burn_us(100);
        TRACE("step1");
        TRACE("step2");
    }
    OOPS_TRACE_STORE::Get().GetRecordTable().Output(std::cout);
}

TEST(ProfilingTrace, Bad) {
    OOPS_TRACE_STORE::Get().Clear();
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
    OOPS_TRACE_STORE::Get().Clear();
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
    OOPS_TRACE_STORE::Get().Clear();
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
    OOPS_TRACE_STORE::Get().Clear();
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
    OOPS_TRACE_STORE::Get().GetRecordTable().Output(std::cout);
}

TEST(ProfilingTrace, Parallel) {
    OOPS_TRACE_STORE::Get().Clear();

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
    OOPS_TRACE_STORE::Get().GetRecordTable().Output(std::cout);
}

TEST(ProfilingTrace, ParallelBusy) {
    OOPS_TRACE_STORE::Get().Clear();

    auto func = [] {
        TRACE_SCOPE(INFO);
        cpu_burn_us(10);
        TRACE("step1");
        for (int i = 0; i < 100; ++i) {
            TRACE_SCOPE(INFO);
            cpu_burn_us(10);
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
    OOPS_TRACE_STORE::Get().GetRecordTable().Output(std::cout);
}

uint64_t GetCpuTimePoint() {
    struct timespec tp;
    if (clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &tp) == 0) {
        return static_cast<uint64_t>(tp.tv_sec * 1000000000ULL) + tp.tv_nsec;
    }
    throw std::runtime_error("GetCpuTimePoint ERROR");
}

TEST(ProfilingTrace, TraceCost) {
    OOPS_TRACE_STORE::Get().Clear();
    std::vector<std::chrono::steady_clock::time_point> tp_time;
    std::vector<uint64_t> cpu_time;
    tp_time.emplace_back();
    cpu_time.emplace_back();
    auto t1 = std::chrono::steady_clock::now();
    for (size_t i = 0; i < 10000; ++i) {
        tp_time.back() = std::chrono::steady_clock::now();
        cpu_time.back() = GetCpuTimePoint();
        tp_time.back() = std::chrono::steady_clock::now();
        cpu_time.back() = GetCpuTimePoint();
    }
    auto t2 = std::chrono::steady_clock::now();
    for (size_t i = 0; i < 10000; ++i) {
        TRACE_SCOPE(INFO);
        TRACE("LOOP");
    }
    auto t3 = std::chrono::steady_clock::now();
    std::cout << std::chrono::duration<double>(t2 - t1).count() << std::endl;
    std::cout << std::chrono::duration<double>(t3 - t2).count() << std::endl;
}
