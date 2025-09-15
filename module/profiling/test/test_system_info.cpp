#include <thread>

#include "gtest/gtest.h"

#include "oops/system_info.h"

using namespace oops;

TEST(ProfilingSystemInfo, Status) { std::cout << status::Get(); }

TEST(ProfilingSystemInfo, Status2) { std::cout << status::Get(status::Field::VM_RSS | status::Field::VM_HWM); }

TEST(ProfilingSystemInfo, Lscpu) { std::cout << lscpu::Get(); }
