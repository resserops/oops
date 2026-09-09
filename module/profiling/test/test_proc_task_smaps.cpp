#include <iostream>

#include "gtest/gtest.h"

#include "oops/proc/task/smaps.h"

TEST(ProfilingSmaps, Smaps) {
    using namespace oops;
    std::cout << proc::task::smaps::Get(proc::task::smaps::Field::VMA);
    std::cout << proc::task::smaps::Get(proc::task::smaps::Field::RSS);
    std::cout << proc::task::smaps::Get(proc::task::smaps::Field::RSS | proc::task::smaps::Field::VMA);
}
