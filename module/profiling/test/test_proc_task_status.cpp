#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "oops/proc/task/status.h"
#include "oops/str.h"

using namespace testing;
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

TEST(ProfilingStatus, StatusParse) {
    namespace status = oops::proc::task::status;

    const fs::path STATUS_CASE{CASE_DIR / "iZbp159dyz8itphnzxoylkZ" / "status.txt"};
    std::ifstream ifs{STATUS_CASE};
    ASSERT_TRUE(ifs.is_open());

    auto info{status::Get(ifs)};
    EXPECT_EQ(info.parsed, ~status::FieldMask{});

    // 标识
    EXPECT_EQ(info.name, "load_cpu_steppe");
    EXPECT_EQ(info.umask, 0002);
    EXPECT_EQ(0666 & ~info.umask, 0664);
    EXPECT_EQ(info.state, "S (sleeping)");
    EXPECT_EQ(info.tgid, 1071733);
    EXPECT_EQ(info.ngid, 0);
    EXPECT_EQ(info.pid, 1071733);
    EXPECT_EQ(info.ppid, 1071503);
    EXPECT_EQ(info.tracer_pid, 0);
    EXPECT_EQ(info.uid.real, 1000);
    EXPECT_EQ(info.uid.effective, 1000);
    EXPECT_EQ(info.uid.saved_set, 1000);
    EXPECT_EQ(info.uid.filesystem, 1000);
    EXPECT_EQ(info.gid.real, 1000);
    EXPECT_EQ(info.gid.effective, 1000);
    EXPECT_EQ(info.gid.saved_set, 1000);
    EXPECT_EQ(info.gid.filesystem, 1000);
    EXPECT_EQ(info.fd_size, 256);
    EXPECT_THAT(info.groups, ElementsAre(27, 100, 1000));

    // 命名空间ID
    EXPECT_THAT(info.ns_tgid, ElementsAre(1071733));
    EXPECT_THAT(info.ns_pid, ElementsAre(1071733));
    EXPECT_THAT(info.ns_pgid, ElementsAre(1071733));
    EXPECT_THAT(info.ns_sid, ElementsAre(1071503));

    // 线程属性
    EXPECT_FALSE(info.kthread);

    // 内存统计
    EXPECT_EQ(info.vm_peak.Count(), 14660);
    EXPECT_EQ(info.vm_size.Count(), 14660);
    EXPECT_EQ(info.vm_hwm.Count(), 4068);
    EXPECT_EQ(info.vm_rss.Count(), 4068);
    EXPECT_EQ(info.rss_anon.Count(), 216);
    EXPECT_EQ(info.rss_file.Count(), 3852);
    EXPECT_EQ(info.rss_shmem.Count(), 0);
    EXPECT_EQ(info.vm_data.Count(), 8460);
    EXPECT_EQ(info.vm_stk.Count(), 132);
    EXPECT_EQ(info.vm_exe.Count(), 88);
    EXPECT_EQ(info.vm_lib.Count(), 3712);
    EXPECT_EQ(info.vm_pte.Count(), 60);
    EXPECT_EQ(info.vm_swap.Count(), 0);
    EXPECT_EQ(info.hugetlb_pages.Count(), 0);

    // 杂项
    EXPECT_FALSE(info.core_dumping);
    EXPECT_TRUE(info.thp_enabled);
    EXPECT_EQ(info.untag_mask, 0xffffffffffffffff);
    EXPECT_EQ(info.threads, 2);

    // 信号
    EXPECT_EQ(info.sig_q.count, 0);
    EXPECT_EQ(info.sig_q.limit, 28836);
    EXPECT_EQ(info.sig_pnd, 0);
    EXPECT_EQ(info.sig_cgt, 0x100000000);

    // 能力
    EXPECT_EQ(info.cap_inh, 0);
    EXPECT_EQ(info.cap_bnd, 0x1ffffffffff);

    // 安全
    EXPECT_FALSE(info.no_new_privs);
    EXPECT_EQ(info.seccomp, 0);
    EXPECT_EQ(info.seccomp_filters, 0);
    EXPECT_EQ(info.speculation_store_bypass, "vulnerable");
    EXPECT_EQ(info.speculation_indirect_branch, "always enabled");

    // 亲和性
    EXPECT_EQ(info.cpus_allowed.size(), 4);
    EXPECT_THAT(info.cpus_allowed, ElementsAre(true, true, false, false));
    EXPECT_EQ(info.cpus_allowed_list, "0-1");

    EXPECT_EQ(info.mems_allowed.size(), 1024);
    EXPECT_TRUE(info.mems_allowed[0]);
    EXPECT_EQ(std::count(info.mems_allowed.begin(), info.mems_allowed.end(), true), 1);
    EXPECT_EQ(info.mems_allowed_list, "0");

    // 上下文切换
    EXPECT_EQ(info.voluntary_ctxt_switches, 283);
    EXPECT_EQ(info.nonvoluntary_ctxt_switches, 130);

    // x86硬件级安全特性
    EXPECT_TRUE(info.x86_thread_features.empty());
    EXPECT_TRUE(info.x86_thread_features_locked.empty());
}

TEST(ProfilingStatus, StatusFormat) {
    namespace status = oops::proc::task::status;

    const fs::path STATUS_CASE{CASE_DIR / "iZbp159dyz8itphnzxoylkZ" / "status.txt"};
    std::ifstream ifs{STATUS_CASE};
    ASSERT_TRUE(ifs.is_open());
    std::stringstream buf;
    buf << ifs.rdbuf();
    auto expected{SplitAndSqueezeLines(buf.str())};

    ifs.clear(); // 恢复被rdbuf耗尽的流
    ifs.seekg(0);
    auto info{status::Get(ifs)};
    std::ostringstream oss;
    oss << info;
    auto actual{SplitAndSqueezeLines(oss.str())};

    ASSERT_EQ(actual.size(), expected.size());
    for (std::size_t i{0}; i < actual.size(); ++i) {
        EXPECT_EQ(actual[i], expected[i]);
    }
}
