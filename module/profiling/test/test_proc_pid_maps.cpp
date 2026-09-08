#include <iostream>

#include "gtest/gtest.h"

#include "oops/proc/pid/maps.h"

TEST(ProfilingMaps, Maps) {
    using namespace oops;
    std::cout << proc::pid::maps::Get();
}
