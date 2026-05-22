#include <chrono>
#include <thread>

#include "gtest/gtest.h"

#include "oops/cpu_timer.h"
#include "oops/system_info.h"

TEST(ProfilingSystemInfo, Status) {
    using namespace oops::proc;
    auto t1 = std::chrono::steady_clock::now();
    auto k = status::Get();
    auto t2 = std::chrono::steady_clock::now();
    std::cout << "Time: " << std::chrono::duration<double>{t2 - t1}.count() << std::endl;
    std::cout << k << std::endl;
}

TEST(ProfilingSystemInfo, Status2) {
    using namespace oops::proc;
    auto t1 = std::chrono::steady_clock::now();
    auto k = status::Get(status::Field::VM_RSS | status::Field::VM_HWM);
    auto t2 = std::chrono::steady_clock::now();
    std::cout << "Time: " << std::chrono::duration<double>{t2 - t1}.count() << std::endl;
    std::cout << k << std::endl;
}

TEST(ProfilingSystemInfo, Status3) {
    using namespace oops::proc;
    auto t1 = std::chrono::steady_clock::now();
    auto k = status::Get();
    auto t2 = std::chrono::steady_clock::now();
    std::cout << "Time: " << std::chrono::duration<double>{t2 - t1}.count() << std::endl;
    std::cout << k << std::endl;
}

TEST(ProfilingSystemInfo, Lscpu) {
    using namespace oops;
    std::cout << lscpu::Get();
}

TEST(ProfilingSystemInfo, Maps) {
    using namespace oops;
    std::cout << proc::maps::Get();
}

TEST(ProfilingSystemInfo, Smaps) {
    using namespace oops;
    std::cout << proc::smaps::Get(proc::smaps::Field::VMA);
    std::cout << proc::smaps::Get(proc::smaps::Field::RSS);
    std::cout << proc::smaps::Get(proc::smaps::Field::RSS | proc::smaps::Field::VMA);
}

TEST(ProfilingSystemInfo, SmapsRollup) {
    using namespace oops;
    std::cout << proc::smaps_rollup::Get();
}
