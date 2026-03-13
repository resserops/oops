#include <cassert>
#include <chrono>
#include <cstring>
#include <fstream>
#include <string>
#include <variant>

#include <unistd.h>

namespace oops {
inline auto GetTicksPerSec() {
    // 每秒时钟滴答数，系统启动后固定
    static const auto TICKS_PER_SEC{sysconf(_SC_CLK_TCK)};
    return TICKS_PER_SEC;
}

class CpuTimer {
    struct CpuTicks {
        std::uintmax_t TotalTicks() const { return user_ticks + kernel_ticks; };
        std::uintmax_t user_ticks;
        std::uintmax_t kernel_ticks;
    };

    using CpuTimePoint = std::variant<struct timespec, CpuTicks, std::monostate>;

public:
    struct Duration {
        double CpuUsage() const { return CpuTime() / ElapsedTime(); }
        double CpuUsagePct() const { return 100 * CpuUsage(); }
        double CpuTime() const { return cpu_time; }
        double ElapsedTime() const { return elapsed_time; }

        double cpu_time{};
        double elapsed_time{};
    };

    CpuTimer() : cpu_t0_{GetCpuTimePoint()}, elapsed_t0_{std::chrono::steady_clock::now()} {}
    CpuTimer(pid_t pid)
        : stat_path_{"/proc/" + std::to_string(pid) + "/stat"}, cpu_t0_{GetCpuTimePoint()},
          elapsed_t0_{std::chrono::steady_clock::now()} {}

    void Reset() {
        cpu_t0_ = GetCpuTimePoint();
        elapsed_t0_ = std::chrono::steady_clock::now();
    }

    Duration Lap() {
        CpuTimePoint cpu_t1{GetCpuTimePoint()};
        auto elapsed_t1{std::chrono::steady_clock::now()};

        double cpu_s{GetCpuDuration(cpu_t1)};
        double elapsed_s{std::chrono::duration<double>(elapsed_t1 - elapsed_t0_).count()};

        cpu_t0_ = cpu_t1;
        elapsed_t0_ = elapsed_t1;
        return {cpu_s, elapsed_s};
    }

    Duration Peek() const {
        CpuTimePoint cpu_t1{GetCpuTimePoint()};
        auto elapsed_t1{std::chrono::steady_clock::now()};

        double cpu_s{GetCpuDuration(cpu_t1)};
        double elapsed_s{std::chrono::duration<double>(elapsed_t1 - elapsed_t0_).count()};
        return {cpu_s, elapsed_s};
    }

    auto GetElapsedT0() const { return elapsed_t0_; }

private:
    CpuTimePoint GetCpuTimePoint() const {
        if (stat_path_.empty()) {
            // 内核实时计数器获取cpu时间，纳秒级精度
            struct timespec spec;
            clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &spec);
            return spec;
        }

        // 文件快照(/proc/pid/stat)获取cpu时间，Ticks级精度(约10ms)
        // 解析效率要求高，使用栈缓冲
        char buf[512];
        std::FILE *f{std::fopen(stat_path_.c_str(), "r")};
        if (!f) {
            return std::monostate{}; // 无法打开文件，返回空类型
        }

        auto ret{std::fgets(buf, sizeof(buf), f)};
        std::fclose(f);
        if (ret == nullptr) {
            return std::monostate{}; // 读取失败，返回空类型
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

    double GetCpuDuration(const CpuTimePoint &cpu_t1) const {
        if (stat_path_.empty()) {
            auto spec_t0{std::get<struct timespec>(cpu_t0_)};
            auto spec_t1{std::get<struct timespec>(cpu_t1)};
            return (spec_t1.tv_sec - spec_t0.tv_sec) + (spec_t1.tv_nsec - spec_t0.tv_nsec) / 1e9;
        }
        // 从文件读取cpu时间可能失败，失败情况返回0
        auto *cpu_ticks_t0{std::get_if<CpuTicks>(&cpu_t0_)};
        auto *cpu_ticks_t1{std::get_if<CpuTicks>(&cpu_t1)};
        if (cpu_ticks_t0 == nullptr || cpu_ticks_t1 == nullptr) {
            return 0;
        }
        return static_cast<double>(cpu_ticks_t1->TotalTicks() - cpu_ticks_t0->TotalTicks()) / GetTicksPerSec();
    }

    const std::string stat_path_;
    CpuTimePoint cpu_t0_;
    std::chrono::steady_clock::time_point elapsed_t0_;
};
} // namespace oops
