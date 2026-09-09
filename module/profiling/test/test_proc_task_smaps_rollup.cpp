#include <iostream>

#include "gtest/gtest.h"

#include "oops/proc/task/smaps_rollup.h"

TEST(ProfilingSmapsRollup, SmapsRollup) {
    using namespace oops;
    std::cout << proc::task::smaps_rollup::Get();
}
