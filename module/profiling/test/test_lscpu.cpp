#include <iostream>

#include "gtest/gtest.h"

#include "oops/lscpu.h"

TEST(ProfilingLscpu, Lscpu) {
    using namespace oops;
    std::cout << lscpu::Get();
}
