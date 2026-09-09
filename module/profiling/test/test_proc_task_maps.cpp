#include <iostream>

#include "gtest/gtest.h"

#include "oops/proc/task/maps.h"

TEST(ProfilingMaps, Maps) {
    using namespace oops;
    std::cout << proc::task::maps::Get();
}
