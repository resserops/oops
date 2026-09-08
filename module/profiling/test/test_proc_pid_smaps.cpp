#include <iostream>

#include "gtest/gtest.h"

#include "oops/proc/pid/smaps.h"

TEST(ProfilingSmaps, Smaps) {
    using namespace oops;
    std::cout << proc::pid::smaps::Get(proc::pid::smaps::Field::VMA);
    std::cout << proc::pid::smaps::Get(proc::pid::smaps::Field::RSS);
    std::cout << proc::pid::smaps::Get(proc::pid::smaps::Field::RSS | proc::pid::smaps::Field::VMA);
}
