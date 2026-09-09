#include <chrono>
#include <iostream>

#include "gtest/gtest.h"

#include "oops/proc/task/status.h"

TEST(ProfilingStatus, Status) {
    using namespace oops::proc::task;
    auto t1 = std::chrono::steady_clock::now();
    auto k = status::Get();
    auto t2 = std::chrono::steady_clock::now();
    std::cout << "Time: " << std::chrono::duration<double>{t2 - t1}.count() << std::endl;
    std::cout << k << std::endl;
}

TEST(ProfilingStatus, Status2) {
    using namespace oops::proc::task;
    auto t1 = std::chrono::steady_clock::now();
    auto k = status::Get(status::Field::VM_RSS | status::Field::VM_HWM);
    auto t2 = std::chrono::steady_clock::now();
    std::cout << "Time: " << std::chrono::duration<double>{t2 - t1}.count() << std::endl;
    std::cout << k << std::endl;
}

TEST(ProfilingStatus, Status3) {
    using namespace oops::proc::task;
    auto t1 = std::chrono::steady_clock::now();
    auto k = status::Get();
    auto t2 = std::chrono::steady_clock::now();
    std::cout << "Time: " << std::chrono::duration<double>{t2 - t1}.count() << std::endl;
    std::cout << k << std::endl;
}
