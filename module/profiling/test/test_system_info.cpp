#include <chrono>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <thread>
#include <vector>

#include "gtest/gtest.h"

#include "oops/cpu_timer.h"
#include "oops/str.h"
#include "oops/system_info.h"

namespace fs = std::filesystem;
static const fs::path CASE_DIR{OOPS_PROFILING_CASE_DIR};

static std::vector<std::string> SplitAndSqueezeLines(std::string_view s) {
    std::vector<std::string> res;
    std::istringstream iss{std::string{s}};
    std::string buf;
    while (std::getline(iss, buf)) {
        res.push_back(oops::RemoveIf(buf, oops::IsSpace));
    }
    return res;
}

TEST(ProfilingSystemInfo, Status) {
    using namespace oops::proc;
    auto t1 = std::chrono::steady_clock::now();
    auto k = status::Get();
    auto t2 = std::chrono::steady_clock::now();
    std::cout << "Time: " << std::chrono::duration<double>{t2 - t1}.count() << std::endl;
    std::cout << k << std::endl;
}

TEST(ProfilingSystemInfo, Status2) {
    using namespace oops::proc;
    auto t1 = std::chrono::steady_clock::now();
    auto k = status::Get(status::Field::VM_RSS | status::Field::VM_HWM);
    auto t2 = std::chrono::steady_clock::now();
    std::cout << "Time: " << std::chrono::duration<double>{t2 - t1}.count() << std::endl;
    std::cout << k << std::endl;
}

TEST(ProfilingSystemInfo, Status3) {
    using namespace oops::proc;
    auto t1 = std::chrono::steady_clock::now();
    auto k = status::Get();
    auto t2 = std::chrono::steady_clock::now();
    std::cout << "Time: " << std::chrono::duration<double>{t2 - t1}.count() << std::endl;
    std::cout << k << std::endl;
}

TEST(ProfilingSystemInfo, Lscpu) {
    using namespace oops;
    std::cout << lscpu::Get();
}

TEST(ProfilingSystemInfo, Maps) {
    using namespace oops;
    std::cout << proc::maps::Get();
}

TEST(ProfilingSystemInfo, Smaps) {
    using namespace oops;
    std::cout << proc::smaps::Get(proc::smaps::Field::VMA);
    std::cout << proc::smaps::Get(proc::smaps::Field::RSS);
    std::cout << proc::smaps::Get(proc::smaps::Field::RSS | proc::smaps::Field::VMA);
}

TEST(ProfilingSystemInfo, SmapsRollup) {
    using namespace oops;
    std::cout << proc::smaps_rollup::Get();
}

TEST(ProfilingSystemInfo, CpuinfoParse) {
    namespace cpuinfo = oops::proc::cpuinfo;

    const fs::path CPUINFO_CASE{CASE_DIR / "iZbp159dyz8itphnzxoylkZ" / "cpuinfo.txt"};
    std::cout << CPUINFO_CASE << std::endl;
    std::ifstream ifs{CPUINFO_CASE};
    ASSERT_TRUE(ifs.is_open());

    auto info{cpuinfo::Get(ifs)};
    ASSERT_EQ(info.table.size(), 2);

    for (std::size_t i{0}; i < info.table.size(); ++i) {
        const auto &entry{info.table[i]};
        EXPECT_EQ(entry.parsed, ~cpuinfo::FieldMask{});

        // 标识
        EXPECT_EQ(entry.processor, i);
        EXPECT_EQ(entry.vendor_id, "GenuineIntel");
        EXPECT_EQ(entry.cpu_family, 6);
        EXPECT_EQ(entry.model, 85);
        EXPECT_EQ(entry.model_name, "Intel(R) Xeon(R) Platinum");
        EXPECT_EQ(entry.stepping, 4);
        EXPECT_EQ(entry.microcode, 1);

        // 规格
        EXPECT_DOUBLE_EQ(entry.cpu_mhz, 2499.998);
        EXPECT_EQ(entry.cache_size.Count(), 33792);

        // 拓扑
        EXPECT_EQ(entry.physical_id, 0);
        EXPECT_EQ(entry.siblings, 2);
        EXPECT_EQ(entry.core_id, 0);
        EXPECT_EQ(entry.cpu_cores, 1);
        EXPECT_EQ(entry.apicid, i);
        EXPECT_EQ(entry.initial_apicid, i);

        // 特性
        EXPECT_TRUE(entry.fpu);
        EXPECT_TRUE(entry.fpu_exception);
        EXPECT_EQ(entry.cpuid_level, 22);
        EXPECT_TRUE(entry.wp);
        EXPECT_EQ(entry.flags.size(), 81);
        EXPECT_EQ(entry.flags.front(), "fpu");
        EXPECT_EQ(entry.flags.back(), "arat");
        EXPECT_EQ(entry.bugs.size(), 13);
        EXPECT_EQ(entry.bugs.front(), "cpu_meltdown");
        EXPECT_EQ(entry.bugs.back(), "its");

        // 其它
        EXPECT_DOUBLE_EQ(entry.bogomips, 4999.99);
        EXPECT_EQ(entry.clflush_size, 64);
        EXPECT_EQ(entry.cache_alignment, 64);
        EXPECT_EQ(entry.address_sizes, "46 bits physical, 48 bits virtual");
        EXPECT_TRUE(entry.power_management.empty());
    }
}

TEST(ProfilingSystemInfo, CpuinfoFormat) {
    namespace cpuinfo = oops::proc::cpuinfo;

    const fs::path CPUINFO_CASE{CASE_DIR / "iZbp159dyz8itphnzxoylkZ" / "cpuinfo.txt"};
    std::ifstream ifs{CPUINFO_CASE};
    ASSERT_TRUE(ifs.is_open());
    std::stringstream buf;
    buf << ifs.rdbuf();
    auto expected{SplitAndSqueezeLines(buf.str())};

    ifs.clear(); // 恢复被rdbuf读取耗尽的流
    ifs.seekg(0);
    auto info{cpuinfo::Get(ifs)};
    std::ostringstream oss;
    oss << info;
    auto actual{SplitAndSqueezeLines(oss.str())};

    ASSERT_EQ(actual.size(), expected.size());
    for (std::size_t i{0}; i < actual.size(); ++i) {
        EXPECT_EQ(actual[i], expected[i]);
    }
}
