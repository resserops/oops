#pragma once
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
    static const auto TICKS_PER_SEC{sysconf(_SC_CLK_TCK)}; // TODO(resserops): 异常判断
    return TICKS_PER_SEC;
}

template <typename T>
struct IsTimePoint : std::false_type {};
template <typename Clock, typename Duration>
struct IsTimePoint<std::chrono::time_point<Clock, Duration>> : std::true_type {};
template <typename T>
constexpr bool IS_TIME_POINT{IsTimePoint<T>::value};

template <typename Dst, typename Src>
constexpr Dst TimePointCast(const Src &src) noexcept {
    static_assert(IS_TIME_POINT<Src>, "source type must be a std::chrono::time_point");
    static_assert(IS_TIME_POINT<Dst>, "destination type must be a std::chrono::time_point");

    auto src_duration{src.time_since_epoch()}; // 获取时间起点epoch至时间点src的时间间隔
    auto dst_duration{std::chrono::duration_cast<typename Dst::duration>(src_duration)};
    return Dst{dst_duration};
}

class FILEGuard {
public:
    FILEGuard() = default;
    explicit FILEGuard(std::FILE *f) noexcept : f_{f} {}
    FILEGuard(const char *path, const char *mode) noexcept : f_{std::fopen(path, mode)} {}

    // 禁止复制，允许移动
    FILEGuard(const FILEGuard &) = delete;
    FILEGuard(FILEGuard &&other) noexcept : f_{other.f_} { other.f_ = nullptr; }

    ~FILEGuard() noexcept { Close(); }

    FILEGuard &operator=(const FILEGuard &) = delete;
    FILEGuard &operator=(FILEGuard &&other) noexcept {
        std::swap(f_, other.f_);
        return *this;
    }

    explicit operator bool() const noexcept { return f_ != nullptr; }
    std::FILE *Get() const noexcept { return f_; }

    void Close() noexcept {
        if (f_ != nullptr) {
            std::fclose(f_);
            f_ = nullptr;
        }
    }

private:
    std::FILE *f_{nullptr};
};

// 定义不同维度的CpuClock，接口和chrono::clock保持一致
struct SelfCpuClock {
    using duration = std::chrono::nanoseconds;
    using rep = duration::rep;
    using period = duration::period;
    using time_point = std::chrono::time_point<SelfCpuClock, duration>;

    static constexpr bool is_steady{true};
    static time_point now() {
        struct timespec spec;
        if (clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &spec) != 0) {
            throw std::runtime_error("SelfCpuClock::now() failed to get CLOCK_PROCESS_CPUTIME_ID");
        }
        return time_point{std::chrono::seconds{spec.tv_sec} + std::chrono::nanoseconds{spec.tv_nsec}};
    }
};

class PidCpuClock {
public:
    using duration = std::chrono::duration<double>;
    using rep = duration::rep;
    using period = duration::period;
    using time_point = std::chrono::time_point<PidCpuClock, duration>;

    static constexpr bool is_steady{true};
    static constexpr auto PERIOD_PER_SEC{duration{std::chrono::seconds{1}}.count()}; // 必须是无损转换

    explicit PidCpuClock(pid_t pid) : stat_path_{"/proc/" + std::to_string(pid) + "/stat"} {}
    time_point now() const {
        // 文件快照(/proc/pid/stat)获取进程或system cpu时间，Ticks级精度(约10ms)
        char buf[512];
        FILEGuard f(stat_path_.c_str(), "r");
        if (!f) {
            throw std::runtime_error("");
        }
        auto res{std::fgets(buf, sizeof(buf), f.Get())};
        f.Close();
        if (res == nullptr) {
            // 异常处理
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
        auto ticks{std::strtoull(p + 1, &p, 10)};

        // 匹配第15个字段kernel_ticks
        ticks += std::strtoull(p + 1, nullptr, 10);
        return time_point{duration{static_cast<rep>(ticks) * PERIOD_PER_SEC / GetTicksPerSec()}};
    }

private:
    const std::string stat_path_;
};

struct SystemCpuClock {
    using duration = std::chrono::duration<double>;
    using rep = duration::rep;
    using period = duration::period;
    using time_point = std::chrono::time_point<SystemCpuClock, duration>;

    static constexpr bool is_steady{true};
    static constexpr auto PERIOD_PER_SEC{duration{std::chrono::seconds{1}}.count()}; // 必须是无损转换
    static time_point now() {
        // 文件快照(/proc/stat)获取系统级cpu时间，Ticks级精度(约10ms)
        char buf[512];
        FILEGuard f("/proc/stat", "r");
        if (!f) {
            throw std::runtime_error("");
        }
        auto res{std::fgets(buf, sizeof(buf), f.Get())};
        f.Close();
        if (res == nullptr) {
            throw std::runtime_error("");
        }

        char *p{buf};
        while ((*p < '0') || (*p > '9')) {
            ++p;
        }
        auto ticks{std::strtoull(p, &p, 10)};  // 匹配第1个字段user
        ticks += std::strtoull(p + 1, &p, 10); // 匹配第2个字段nice
        ticks += std::strtoull(p + 1, &p, 10); // 匹配第3个字段system

        // 跳过第4个字段idle和第5个字段iowait
        int rem{2};
        while (rem-- > 0) {
            p = std::strchr(p + 1, ' ');
            assert(p != nullptr);
        }

        ticks += std::strtoull(p + 1, &p, 10); // 匹配第6个字段irq
        ticks += std::strtoull(p + 1, &p, 10); // 匹配第7个字段softirq
        return time_point{duration{static_cast<rep>(ticks) * PERIOD_PER_SEC / GetTicksPerSec()}};
    }
};

class UniCpuClock {
public:
    using duration = std::chrono::duration<double>;
    using rep = duration::rep;
    using period = duration::period;
    using time_point = std::chrono::time_point<UniCpuClock, duration>;
    using CpuClockVar = std::variant<SelfCpuClock, PidCpuClock, SystemCpuClock>;

    static constexpr bool is_steady{true};
    enum class Kind : uint8_t { SELF, PID, SYSTEM };

    UniCpuClock() : kind_{Kind::SELF}, clock_{SelfCpuClock{}} {}
    explicit UniCpuClock(Kind kind, pid_t pid = 0) : kind_{kind} {
        switch (kind) {
        case Kind::SELF: {
            clock_.emplace<SelfCpuClock>();
            break;
        }
        case Kind::SYSTEM: {
            clock_.emplace<SystemCpuClock>();
            break;
        }
        case Kind::PID: {
            if (pid <= 0) {
                throw std::invalid_argument("invalid pid " + std::to_string(pid));
            }
            clock_.emplace<PidCpuClock>(pid);
            break;
        }
        }
    }

    time_point now() const {
        return std::visit([](const auto &clock) { return TimePointCast<time_point>(clock.now()); }, clock_);
    }

private:
    Kind kind_;
    CpuClockVar clock_;
};

// 定义相关定时器
template <typename Clock>
class Timer {
public:
    using TimePoint = typename Clock::time_point;
    using Duration = typename Clock::duration;

    template <typename T = Clock, typename = std::enable_if_t<std::is_default_constructible_v<T>>>
    Timer() : clock_{}, t_start_{clock_.now()}, t_prev_{t_start_} {}

    template <typename... ClockArgs, typename = std::enable_if_t<(sizeof...(ClockArgs) > 0)>>
    explicit Timer(ClockArgs &&...clock_args)
        : clock_{std::forward<ClockArgs>(clock_args)...}, t_start_{clock_.now()}, t_prev_{t_start_} {}

    // 重置计时点
    void Reset() { t_prev_ = clock_.now(); }

    // 计算当前距离前个计时点的耗时，并刷新计时点
    Duration Lap() {
        TimePoint t_now{clock_.now()};
        Duration lap{t_now - t_prev_};
        t_prev_ = t_now;
        return lap;
    }

    // 计算当前距离前个计时点的耗时，不刷新计时点
    Duration Peek() const { return clock_.now() - t_prev_; }
    Duration Total() const { return clock_.now() - t_start_; }
    TimePoint GetStart() const { return t_start_; }
    TimePoint GetPrev() const { return t_prev_; }
    const auto &GetClock() const { return clock_; }

private:
    Clock clock_;
    TimePoint t_start_;
    TimePoint t_prev_;
};

template <typename CpuClock, typename ElapsedClock = std::chrono::steady_clock>
class CpuTimer {
public:
    static_assert(std::is_default_constructible_v<ElapsedClock>);
    struct Duration {
        double ElapsedSeconds() const { return std::chrono::duration<double>{elapsed}.count(); }
        double CpuSeconds() const { return std::chrono::duration<double>{cpu}.count(); }
        double CpuUsage() const { return CpuSeconds() / ElapsedSeconds(); }
        double CpuUsagePct() const { return 100 * CpuUsage(); }

        typename Timer<ElapsedClock>::Duration elapsed{};
        typename Timer<CpuClock>::Duration cpu{};
    };

    // 先计算开销较小的elapsed_time
    template <typename T = CpuClock, typename = std::enable_if_t<std::is_default_constructible_v<T>>>
    CpuTimer() : elapsed_timer_{}, cpu_timer_{} {}

    template <typename... ClockArgs, typename = std::enable_if_t<(sizeof...(ClockArgs) > 0)>>
    explicit CpuTimer(ClockArgs &&...clock_args)
        : elapsed_timer_{}, cpu_timer_{std::forward<ClockArgs>(clock_args)...} {}

    void Reset() {
        elapsed_timer_.Reset();
        cpu_timer_.Reset();
    }

    Duration Lap() { return {elapsed_timer_.Lap(), cpu_timer_.Lap()}; }
    Duration Peek() const { return {elapsed_timer_.Peek(), cpu_timer_.Peek()}; }
    Duration Total() const { return {elapsed_timer_.Total(), cpu_timer_.Total()}; }
    const auto &GetCpuTimer() const { return cpu_timer_; }
    const auto &GetElapsedTimer() const { return elapsed_timer_; }

private:
    Timer<std::chrono::steady_clock> elapsed_timer_;
    Timer<CpuClock> cpu_timer_;
};

using SelfCpuTimer = CpuTimer<SelfCpuClock>;
using PidCpuTimer = CpuTimer<PidCpuClock>;
using SystemCpuTimer = CpuTimer<SystemCpuClock>;
using UniCpuTimer = CpuTimer<UniCpuClock>;
} // namespace oops
