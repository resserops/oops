#include <algorithm>
#include <atomic>
#include <chrono>
#include <csignal>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include <sys/stat.h>
#include <unistd.h>

#include "argparse/argparse.hpp"

#include "oops/cpu_freq.h"
#include "oops/cpu_timer.h"
#include "oops/enum_bitset.h"
#include "oops/once.h"
#include "oops/proc/meminfo.h"
#include "oops/proc/task/status.h"
#include "oops/storage.h"
#include "oops/str.h"

enum class Mode : uint8_t { PID, SYSTEM_WIDE, FALLBACK };

// 全局配置
struct Args {
    Mode mode{Mode::SYSTEM_WIDE};
    pid_t pid{};
    double itv{};
} ARGS;

// 指标定义
enum class MetricGroup : uint8_t { CPU, MEMORY, COUNT };
oops::EnumBitset<MetricGroup> ENABLED_METRIC_GROUP;

enum class Metrics : uint8_t { CPU_EQ_CORES, CPU_AVG_GHZ, CPU_MIN_GHZ, RSS, HWM, SWAP, COUNT };

struct MetricEntry {
    std::string_view name;
    std::size_t width{6};
    MetricGroup group{};
    bool only_system_wide{false};
};

constexpr MetricEntry METRIC_TABLE[]{{"EqCPUs", 8, MetricGroup::CPU},       {"AvgGHz", 8, MetricGroup::CPU, true},
                                     {"MinGHz", 8, MetricGroup::CPU, true}, {"RSS(G)", 8, MetricGroup::MEMORY},
                                     {"HWM(G)", 8, MetricGroup::MEMORY},    {"Swap(G)", 8, MetricGroup::MEMORY}};
static_assert(std::size(METRIC_TABLE) == oops::ToUnderlying(Metrics::COUNT));

bool ProcExist(pid_t pid) {
    static std::string path{"/proc/" + std::to_string(pid)};
    struct stat buffer;
    return stat(path.c_str(), &buffer) == 0;
}

std::string TimestampToStr(std::chrono::system_clock::time_point time_point) {
    std::time_t time{std::chrono::system_clock::to_time_t(time_point)};
    std::tm *tm{std::localtime(&time)}; // std::localtime不可重入

    auto frac{time_point - std::chrono::time_point_cast<std::chrono::seconds>(time_point)};
    auto frac_us{std::chrono::duration_cast<std::chrono::microseconds>(frac).count()};

    std::ostringstream oss;
    oss << std::put_time(tm, "%Y-%m-%d %H:%M:%S") << '.' << std::setfill('0') << std::setw(6) << frac_us;
    return oss.str();
}

std::string MakeHeaderRow() {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2);
    oss << std::setw(8) << "#"
        << ", ";
    for (std::size_t i{0}; i < oops::ToUnderlying(Metrics::COUNT); ++i) {
        const MetricEntry &entry{METRIC_TABLE[i]};
        if (!ENABLED_METRIC_GROUP.Test(entry.group)) {
            continue;
        }
        if (entry.only_system_wide && ARGS.mode == Mode::PID) {
            continue;
        }
        oss << std::setw(entry.width) << entry.name << ", ";
    }
    std::string res{oss.str()};
    res.pop_back();
    res.pop_back();
    return res;
}

std::string MakeDataRow(const std::vector<double> &values) {
    static std::size_t number{};
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2);
    oss << std::setw(8) << number++ << ", ";
    for (std::size_t i{0}; i < oops::ToUnderlying(Metrics::COUNT); ++i) {
        const MetricEntry &entry{METRIC_TABLE[i]};
        if (!ENABLED_METRIC_GROUP.Test(entry.group)) {
            continue;
        }
        if (entry.only_system_wide && ARGS.mode == Mode::PID) {
            continue;
        }
        oss << std::setw(entry.width) << values[i] << ", ";
    }
    std::string res{oss.str()};
    res.pop_back();
    res.pop_back();
    return res;
}

std::atomic<bool> STOP{false};
void SignalHandler(int sig) {
    STOP.store(true, std::memory_order_relaxed);
    std::signal(sig, SIG_DFL);
}

void RegisterSignalHandler() {
    struct sigaction act {};
    act.sa_handler = SignalHandler;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;

    sigaction(SIGINT, &act, nullptr);  // Ctrl+C
    sigaction(SIGTERM, &act, nullptr); // kill
}

void Measure() {
    std::cout << std::fixed << std::setprecision(2);
    if (ARGS.mode == Mode::SYSTEM_WIDE) {
        std::cout << "tracked pid: system-wide" << std::endl;
    } else {
        std::cout << "tracked pid: " << ARGS.pid << std::endl;
    }
    std::cout << "interval: " << ARGS.itv << "s" << std::endl;

    // itv转换为毫秒定点数
    std::chrono::milliseconds itv{static_cast<std::size_t>(1000 * ARGS.itv)};

    // 注册信号
    RegisterSignalHandler();

    // 系统时钟盖时间戳，单调时钟算时间差
    auto system_now{std::chrono::system_clock::now()};
    oops::UniCpuClock::Kind cpu_timer_kind;
    if (ARGS.mode == Mode::SYSTEM_WIDE) {
        cpu_timer_kind = oops::UniCpuClock::Kind::SYSTEM;
    } else {
        cpu_timer_kind = oops::UniCpuClock::Kind::PID;
    }
    oops::UniCpuTimer cpu_timer(cpu_timer_kind, ARGS.pid);

    std::cout << "timestamp: " << TimestampToStr(system_now) << '\n' << std::endl;
    std::size_t step{0};
    auto start_time{cpu_timer.GetElapsedTimer().GetStart()};
    auto next_time{start_time + (step + 1) * itv};

    std::cout << "monitoring results:" << std::endl;
    std::cout << MakeHeaderRow() << std::endl;

    std::vector<double> values(oops::ToUnderlying(Metrics::COUNT));
    double system_wide_hwm{0.};
    while (!STOP.load(std::memory_order_relaxed) && (ARGS.mode == Mode::SYSTEM_WIDE || ProcExist(ARGS.pid))) {
        std::this_thread::sleep_until(next_time);

        if (ENABLED_METRIC_GROUP.Test(MetricGroup::CPU)) {
            // 等效核数
            values[oops::ToUnderlying(Metrics::CPU_EQ_CORES)] = TRY_OR(cpu_timer.Lap().CpuUsage(), .0);

            // CPU主频
            if (ARGS.mode != Mode::PID) {
                auto freq{oops::cpu_freq::Get()};
                values[oops::ToUnderlying(Metrics::CPU_AVG_GHZ)] = freq.Average() / 1000.;
                values[oops::ToUnderlying(Metrics::CPU_MIN_GHZ)] = freq.Min() / 1000.;
            }
        }

        if (ENABLED_METRIC_GROUP.Test(MetricGroup::MEMORY)) {
            if (ARGS.mode == Mode::SYSTEM_WIDE) {
                using namespace oops::proc::meminfo;
                auto info{Get(Field::ANON_PAGES | Field::MAPPED | Field::SWAP_TOTAL | Field::SWAP_FREE)};
                values[oops::ToUnderlying(Metrics::RSS)] =
                    oops::Storage<double, oops::GiB>{info.anon_pages + info.mapped}.Count();
                system_wide_hwm = std::max(system_wide_hwm, values[oops::ToUnderlying(Metrics::RSS)]);
                values[oops::ToUnderlying(Metrics::HWM)] = system_wide_hwm;
                values[oops::ToUnderlying(Metrics::SWAP)] =
                    oops::Storage<double, oops::GiB>{info.swap_total - info.swap_free}.Count();
            } else {
                using namespace oops::proc::task::status;
                auto info{Get(ARGS.pid, Field::VM_RSS | Field::VM_HWM | Field::VM_SWAP)};
                values[oops::ToUnderlying(Metrics::RSS)] = oops::Storage<double, oops::GiB>{info.vm_rss}.Count();
                values[oops::ToUnderlying(Metrics::HWM)] = oops::Storage<double, oops::GiB>{info.vm_hwm}.Count();
                values[oops::ToUnderlying(Metrics::SWAP)] = oops::Storage<double, oops::GiB>{info.vm_swap}.Count();
            }
        }

        // 打印测量结果
        std::cout << MakeDataRow(values) << std::endl;

        ++step;
        next_time += itv;
    }
}

// 参数解析
void ParseArgs(int argc, char *argv[]) {
    argparse::ArgumentParser program{"monitor", "1.0"};
    program.add_description("measure runtime performance metrics");
    program.add_argument("pid")
        .help("target process id to monitor")
        .nargs(argparse::nargs_pattern::optional)
        .scan<'i', pid_t>();
    program.add_argument("-i", "--itv")
        .help("sampling interval in secondes (e.g., 0.5 for 500ms)")
        .scan<'g', double>()
        .default_value(1.);
    program.add_argument("-m", "--metric")
        .help("comma-separated sequence of metrics to monitor (supported metrics: cpu[c], memory[m])")
        .default_value("all");
    program.add_argument("-f", "--fallback")
        .help("fall back to system-wide acquisition for metrics without pid-wide support")
        .flag();

    try {
        program.parse_args(argc, argv);
    } catch (const std::exception &e) {
        std::cerr << "error: " << e.what() << std::endl;
        std::cerr << program;
        exit(1);
    }

    // pid
    if (program.is_used("pid")) {
        ARGS.pid = program.get<pid_t>("pid");
        if (!ProcExist(ARGS.pid)) {
            std::cerr << "error: process " << ARGS.pid << " not exist" << std::endl;
            exit(1);
        }
        if (program.get<bool>("--fallback")) {
            ARGS.mode = Mode::FALLBACK;
        } else {
            ARGS.mode = Mode::PID;
        }
    } else {
        ARGS.mode = Mode::SYSTEM_WIDE;
    }

    // --itv
    ARGS.itv = program.get<double>("--itv");
    if (ARGS.itv < .01) {
        std::cerr << "error: interval must be greater than 10ms" << std::endl;
        exit(1);
    }

    // --metric
    std::string metric{program.get<std::string>("--metric")};
    auto metric_tokens{oops::Split(metric, ',')};

    for (auto token : metric_tokens) {
        bool matched{false};
        if ((token == "all" || token == "cpu" || token == "c") && !ENABLED_METRIC_GROUP.Test(MetricGroup::CPU)) {
            ENABLED_METRIC_GROUP.Set(MetricGroup::CPU);
            matched = true;
        }
        if ((token == "all" || token == "memory" || token == "mem" || token == "m") &&
            !ENABLED_METRIC_GROUP.Test(MetricGroup::MEMORY)) {
            ENABLED_METRIC_GROUP.Set(MetricGroup::MEMORY);
            matched = true;
        }
        if (!matched) {
            std::cerr << "error: unexpected metric '" << token << "'" << std::endl;
            exit(1);
        }
    }
    if (ENABLED_METRIC_GROUP.none()) {
        std::cerr << "error: no metrics available" << std::endl;
        exit(1);
    }
}

int main(int argc, char *argv[]) {
    ParseArgs(argc, argv);
    Measure();
    return 0;
}
