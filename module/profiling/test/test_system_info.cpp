#include <chrono>
#include <thread>

#include "gtest/gtest.h"

#include "oops/cpu_timer.h"
#include "oops/system_info.h"

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

struct CpuTicks {
    std::uintmax_t TotalTicks() const { return user_ticks + kernel_ticks; };
    std::uintmax_t user_ticks;
    std::uintmax_t kernel_ticks;
};

inline CpuTicks GetCpuTimePoint2() {
    // 文件快照(/proc/pid/stat)获取cpu时间，Ticks级精度(约10ms)
    // 解析效率要求高，使用栈缓冲
    char buf[512];
    std::FILE *f{std::fopen("/proc/self/stat", "r")};
    if (!f) {
        return {}; // 无法打开文件，返回空类型
    }

    auto res{std::fgets(buf, sizeof(buf), f)};
    std::fclose(f);
    if (res == nullptr) {
        return {}; // 读取失败，返回空类型
    }

    // 查找第2个字段的右括号
    char *p{std::strrchr(buf, ')')};
    assert(p != nullptr);
    p += 2;

    // 根据空格数量匹配第14个字段user_ticks
    int rem{11};
    while (rem-- > 0) {
        p = std::strchr(p + 1, ' ');
        assert(p != nullptr);
    }

    CpuTicks cpu_ticks;
    cpu_ticks.user_ticks = std::strtoull(p + 1, &p, 10);

    // 匹配第15个字段kernel_ticks
    cpu_ticks.kernel_ticks = std::strtoull(p + 1, nullptr, 10);
    return cpu_ticks;
}

TEST(ProfilingSystemInfo, Parse) {
    using namespace oops::proc;
    auto t1 = std::chrono::steady_clock::now();
    auto k = GetCpuTimePoint2();
    auto t2 = std::chrono::steady_clock::now();
    std::cout << "Time: " << std::chrono::duration<double>{t2 - t1}.count() << std::endl;
    std::cout << k.TotalTicks() << std::endl;
}

TEST(ProfilingSystemInfo, Lscpu) {
    using namespace oops;
    std::cout << lscpu::Get();
}

TEST(ProfilingSystemInfo, Smaps) {
    using namespace oops;
    std::cout << proc::maps::Get();
}

TEST(ProfilingSystemInfo, SmapsRollup) {
    using namespace oops;
    std::cout << proc::smaps_rollup::Get();
}
