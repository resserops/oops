#include <iostream>

#include "gtest/gtest.h"

#include "oops/proc/pid/smaps_rollup.h"

TEST(ProfilingSmapsRollup, SmapsRollup) {
    using namespace oops;
    std::cout << proc::pid::smaps_rollup::Get();
}
