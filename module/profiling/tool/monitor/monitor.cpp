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

#include "oops/cpu_timer.h"
#include "oops/enum_bitset.h"
#include "oops/once.h"
#include "oops/proc/task/status.h"
#include "oops/str.h"
#include "oops/unit.h"

// 配置全局变量
inline struct Args {
    bool system_wide{};
    pid_t pid{};
    double itv{};
} ARGS;

// 测量全局变量
enum class MetricGroup : uint8_t { CPU, MEMORY, COUNT };
inline oops::EnumBitset<MetricGroup> ENABLED_METRIC_GROUP;

enum class Metrics : uint8_t { CPU_USAGE, RSS, HWM, SWAP, COUNT };
struct MetricEntry {
    std::string_view name;
    std::size_t width{6};
    MetricGroup group{};
};

constexpr MetricEntry METRIC_TABLE[] = {
    {"%Cpu", 6, MetricGroup::CPU},
    {"Rss(G)", 8, MetricGroup::MEMORY},
    {"Hwm(G)", 8, MetricGroup::MEMORY},
    {"Swap(G)", 8, MetricGroup::MEMORY}};
static_assert(std::size(METRIC_TABLE) == oops::ToUnderlying(Metrics::COUNT));

// 测量功能
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
    oss << std::setw(6) << "#"
        << ", ";
    for (auto &entry : METRIC_TABLE) {
        if (ENABLED_METRIC_GROUP.Test(entry.group)) {
            oss << std::setw(entry.width) << entry.name << ", ";
        }
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
    oss << std::setw(6) << number++ << ", ";
    for (std::size_t i{0}; i < oops::ToUnderlying(Metrics::COUNT); ++i) {
        if (ENABLED_METRIC_GROUP.Test(METRIC_TABLE[i].group)) {
            oss << std::setw(METRIC_TABLE[i].width) << values[i] << ", ";
        }
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
    std::cout << "Pid: " << getpid() << std::endl;
    if (ARGS.system_wide) {
        std::cout << "Tracked pid: system-wide" << std::endl;
    } else {
        std::cout << "Tracked pid: " << ARGS.pid << std::endl;
    }
    std::cout << "Interval: " << ARGS.itv << "s" << std::endl;
    std::cout << "Ticks per sec: " << oops::GetTicksPerSec() << std::endl;

    // itv转换为毫秒定点数
    std::chrono::milliseconds itv{static_cast<std::size_t>(1000 * ARGS.itv)};

    // 注册信号
    RegisterSignalHandler();

    // 系统时钟盖时间戳，单调时钟算时间差
    auto system_now{std::chrono::system_clock::now()};
    oops::UniCpuClock::Kind cpu_timer_kind;
    if (ARGS.system_wide) {
        cpu_timer_kind = oops::UniCpuClock::Kind::SYSTEM;
    } else {
        cpu_timer_kind = oops::UniCpuClock::Kind::PID;
    }
    oops::UniCpuTimer cpu_timer(cpu_timer_kind, ARGS.pid);

    std::cout << "Timestamp: " << TimestampToStr(system_now) << '\n' << std::endl;
    std::size_t step{0};
    auto start_time{cpu_timer.GetElapsedTimer().GetStart()};
    auto next_time{start_time + (step + 1) * itv};

    std::cout << "Monitoring results:" << std::endl;
    std::cout << MakeHeaderRow() << std::endl;

    std::vector<double> values(oops::ToUnderlying(Metrics::COUNT));
    while (!STOP.load(std::memory_order_relaxed) && (ARGS.system_wide || ProcExist(ARGS.pid))) {
        std::this_thread::sleep_until(next_time);

        // 根据选项配置完成测量
        if (ENABLED_METRIC_GROUP.Test(MetricGroup::CPU)) { // 有限测量区间指标，再测量单点指标
            double usage_pct{TRY_OR(cpu_timer.Lap().CpuUsagePct(), .0)};
            values[oops::ToUnderlying(Metrics::CPU_USAGE)] = usage_pct;
        }

        if (ENABLED_METRIC_GROUP.Test(MetricGroup::MEMORY)) {
            if (!ARGS.system_wide) {
                using namespace oops::proc::task::status;
                Info info{Get(ARGS.pid, Field::VM_RSS | Field::VM_HWM | Field::VM_SWAP)};
                values[oops::ToUnderlying(Metrics::RSS)] = oops::GiBs<double>{oops::KiBs<>{info.vm_rss}}.Count();
                values[oops::ToUnderlying(Metrics::HWM)] = oops::GiBs<double>{oops::KiBs<>{info.vm_hwm}}.Count();
                values[oops::ToUnderlying(Metrics::SWAP)] = oops::GiBs<double>{oops::KiBs<>{info.vm_swap}}.Count();
            }
        }

        // 打印测量结果，解耦测量和打印，支撑后续二进制格式
        // 注意：输出格式被同目录report.py依赖，修改需同步
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

    try {
        program.parse_args(argc, argv);
    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << std::endl;
        std::cerr << program;
        exit(1);
    }

    // pid
    if (program.is_used("pid")) {
        ARGS.system_wide = false;
        ARGS.pid = program.get<pid_t>("pid");
        if (!ProcExist(ARGS.pid)) {
            std::cerr << "Error: process " << ARGS.pid << " not exist" << std::endl;
            exit(1);
        }
    } else {
        ARGS.system_wide = true;
    }

    // --itv
    ARGS.itv = program.get<double>("--itv");
    if (ARGS.itv < .01) {
        std::cerr << "Error: interval must be greater than 10ms" << std::endl;
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
            std::cerr << "Error: unexpected metric '" << token << "'" << std::endl;
            exit(1);
        }
    }
    if (ENABLED_METRIC_GROUP.none()) {
        std::cerr << "Error: no metrics available" << std::endl;
        exit(1);
    }
}

int main(int argc, char *argv[]) {
    ParseArgs(argc, argv);
    Measure();
    return 0;
}
