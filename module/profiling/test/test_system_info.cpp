#include <thread>

#include "gtest/gtest.h"

#include "oops/system_info.h"

TEST(ProfilingSystemInfo, Status) {
    using namespace oops::proc;
    std::cout << status::Get();
}

TEST(ProfilingSystemInfo, Status2) {
    std::cout << oops::proc::status::Get(oops::proc::status::Field::VM_RSS | oops::proc::status::Field::VM_HWM);
}

TEST(ProfilingSystemInfo, Lscpu) {
    using namespace oops;
    std::cout << lscpu::Get();
}
