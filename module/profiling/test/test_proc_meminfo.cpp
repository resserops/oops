#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include "gtest/gtest.h"

#include "oops/proc/meminfo.h"
#include "oops/str.h"

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

TEST(ProfilingMeminfo, MeminfoParse) {
    namespace meminfo = oops::proc::meminfo;

    const fs::path MEMINFO_CASE{CASE_DIR / "iZbp159dyz8itphnzxoylkZ" / "meminfo.txt"};
    std::ifstream ifs{MEMINFO_CASE};
    ASSERT_TRUE(ifs.is_open());

    auto info{meminfo::Get(ifs)};
    EXPECT_EQ(info.parsed, ~meminfo::FieldMask{});

    // 基础信息
    EXPECT_EQ(info.mem_total.Count(), 7445736);
    EXPECT_EQ(info.mem_free.Count(), 1136528);
    EXPECT_EQ(info.mem_available.Count(), 3351292);

    // 页面缓存
    EXPECT_EQ(info.buffers.Count(), 114164);
    EXPECT_EQ(info.cached.Count(), 2258132);
    EXPECT_EQ(info.swap_cached.Count(), 0);

    // LRU
    EXPECT_EQ(info.active.Count(), 4782480);
    EXPECT_EQ(info.inactive.Count(), 1141924);
    EXPECT_EQ(info.active_anon.Count(), 3489808);
    EXPECT_EQ(info.inactive_anon.Count(), 73748);
    EXPECT_EQ(info.active_file.Count(), 1292672);
    EXPECT_EQ(info.inactive_file.Count(), 1068176);
    EXPECT_EQ(info.unevictable.Count(), 28968);
    EXPECT_EQ(info.mlocked.Count(), 27432);

    // 交换区
    EXPECT_EQ(info.swap_total.Count(), 4194300);
    EXPECT_EQ(info.swap_free.Count(), 4194300);
    EXPECT_EQ(info.zswap.Count(), 0);
    EXPECT_EQ(info.zswapped.Count(), 0);

    // 页面统计
    EXPECT_EQ(info.dirty.Count(), 36);
    EXPECT_EQ(info.writeback.Count(), 0);
    EXPECT_EQ(info.anon_pages.Count(), 3581096);
    EXPECT_EQ(info.mapped.Count(), 597540);
    EXPECT_EQ(info.shmem.Count(), 2656);

    // 内核
    EXPECT_EQ(info.kreclaimable.Count(), 147852);
    EXPECT_EQ(info.slab.Count(), 222280);
    EXPECT_EQ(info.sreclaimable.Count(), 147852);
    EXPECT_EQ(info.sunreclaim.Count(), 74428);
    EXPECT_EQ(info.kernel_stack.Count(), 7840);
    EXPECT_EQ(info.page_tables.Count(), 62584);
    EXPECT_EQ(info.sec_page_tables.Count(), 0);

    // 临时缓冲区
    EXPECT_EQ(info.nfs_unstable.Count(), 0);
    EXPECT_EQ(info.bounce.Count(), 0);
    EXPECT_EQ(info.writeback_tmp.Count(), 0);

    // 提交
    EXPECT_EQ(info.commit_limit.Count(), 7917168);
    EXPECT_EQ(info.committed_as.Count(), 4397948);

    // vmalloc
    EXPECT_EQ(info.vmalloc_total.Count(), 34359738367ULL);
    EXPECT_EQ(info.vmalloc_used.Count(), 26244);
    EXPECT_EQ(info.vmalloc_chunk.Count(), 0);

    // Per-CPU变量和损坏内存
    EXPECT_EQ(info.percpu.Count(), 1592);
    EXPECT_EQ(info.hardware_corrupted.Count(), 0);

    // 透明大页
    EXPECT_EQ(info.anon_huge_pages.Count(), 0);
    EXPECT_EQ(info.shmem_huge_pages.Count(), 0);
    EXPECT_EQ(info.shmem_pmd_mapped.Count(), 0);
    EXPECT_EQ(info.file_huge_pages.Count(), 0);
    EXPECT_EQ(info.file_pmd_mapped.Count(), 0);

    // 内存加密虚拟机
    EXPECT_EQ(info.unaccepted.Count(), 0);

    // 标准大页
    EXPECT_EQ(info.huge_pages_total, 0);
    EXPECT_EQ(info.huge_pages_free, 0);
    EXPECT_EQ(info.huge_pages_rsvd, 0);
    EXPECT_EQ(info.huge_pages_surp, 0);
    EXPECT_EQ(info.huge_page_size.Count(), 2048);
    EXPECT_EQ(info.hugetlb.Count(), 0);

    // 直接映射（x86架构）
    EXPECT_EQ(info.direct_map_4k.Count(), 144664);
    EXPECT_EQ(info.direct_map_2m.Count(), 5849088);
    EXPECT_EQ(info.direct_map_1g.Count(), 2097152);
}

TEST(ProfilingMeminfo, MeminfoFormat) {
    namespace meminfo = oops::proc::meminfo;

    const fs::path MEMINFO_CASE{CASE_DIR / "iZbp159dyz8itphnzxoylkZ" / "meminfo.txt"};
    std::ifstream ifs{MEMINFO_CASE};
    ASSERT_TRUE(ifs.is_open());
    std::stringstream buf;
    buf << ifs.rdbuf();
    auto expected{SplitAndSqueezeLines(buf.str())};

    ifs.clear(); // 恢复被rdbuf耗尽的流
    ifs.seekg(0);
    auto info{meminfo::Get(ifs)};
    std::ostringstream oss;
    oss << info;
    auto actual{SplitAndSqueezeLines(oss.str())};

    ASSERT_EQ(actual.size(), expected.size());
    for (std::size_t i{0}; i < actual.size(); ++i) {
        EXPECT_EQ(actual[i], expected[i]);
    }
}
