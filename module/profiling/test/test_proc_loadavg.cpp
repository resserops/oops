#include <sstream>
#include <string>

#include "gtest/gtest.h"

#include "oops/proc/loadavg.h"

TEST(ProfilingLoadavg, LoadavgGet) {
    std::istringstream iss{"0.13 0.08 0.02 1/550 28473\n"};
    auto info{oops::proc::loadavg::Get(iss)};

    EXPECT_TRUE(info.parsed);
    EXPECT_EQ(info.load1, 0.13);
    EXPECT_EQ(info.load5, 0.08);
    EXPECT_EQ(info.load15, 0.02);
    EXPECT_EQ(info.sched_entities.running, 1);
    EXPECT_EQ(info.sched_entities.total, 550);
    EXPECT_EQ(info.last_pid, 28473);
}

TEST(ProfilingLoadavg, LoadavgGetFails) {
    namespace loadavg = oops::proc::loadavg;

    std::istringstream iss{"0.13 0.08 0.02 1/550\n"};
    EXPECT_FALSE(loadavg::Get(iss).parsed);

    iss = std::istringstream{"0.13 0.08 0.02 1:550 28473\n"};
    EXPECT_FALSE(loadavg::Get(iss).parsed);

    iss = std::istringstream{"\n"};
    EXPECT_FALSE(loadavg::Get(iss).parsed);
}

TEST(ProfilingLoadavg, LoadavgOutput) {
    const std::string EXPECTED{"0.13 0.08 0.02 1/550 28473\n"};

    std::istringstream iss{EXPECTED};
    auto info{oops::proc::loadavg::Get(iss)};
    ASSERT_TRUE(info.parsed);

    std::ostringstream oss;
    oss << info;
    EXPECT_EQ(oss.str(), EXPECTED);
}
