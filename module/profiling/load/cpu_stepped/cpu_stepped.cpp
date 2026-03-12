#include <algorithm>
#include <atomic>
#include <cassert>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <thread>
#include <vector>

#include <unistd.h>

#include "argparse/argparse.hpp"

#include "oops/cpu_timer.h"

// 参数解析阶段依赖
std::vector<int> ParseIntList(const std::string &str) {
    std::vector<int> res;
    std::istringstream oss{str};
    std::string buf;
    while (std::getline(oss, buf, ',')) {
        if (!buf.empty()) {
            res.push_back(std::stoi(buf));
        }
    }
    return res;
}

struct Args {
    std::chrono::milliseconds itv;
    std::optional<int> step_num;
    std::vector<int> schedule;
};

Args ParseArgs(int argc, char *argv[]) {
    // 配置参数解析器
    argparse::ArgumentParser program{"load_cpu_stepped", "1.0"};
    program.add_argument("-i", "--itv")
        .help("duration of each load step in milliseconds")
        .default_value(1000)
        .scan<'i', int>();
    program.add_argument("-s", "--schedule")
        .help("comma-separated sequence of active core counts for each load step")
        .default_value("0,1");
    program.add_argument("-n", "--step-num").help("total number of steps").scan<'i', int>();

    // 解析参数
    Args args;
    try {
        program.parse_args(argc, argv);

        // --itv
        int itv_ms = program.get<int>("--itv");
        if (itv_ms <= 0) {
            throw std::invalid_argument("interval must be greater than 0");
        }
        args.itv = std::chrono::milliseconds{itv_ms};
        std::cout << "Interval: " << args.itv.count() << " ms" << std::endl;

        // --step-num
        args.step_num = program.present<int>("--step-num");
        if (args.step_num.has_value()) {
            std::cout << "Step num: " << (*args.step_num) << std::endl;
        } else {
            std::cout << "Step num: No limit" << std::endl;
        }

        // --schedule
        args.schedule = ParseIntList(program.get<std::string>("--schedule"));
        if (args.schedule.empty()) {
            args.schedule.push_back(0);
        } else {
            const int hardware_cores{static_cast<int>(std::thread::hardware_concurrency())};
            for (auto &active_cores : args.schedule) {
                if (active_cores > hardware_cores) {
                    std::cout << "Active cores " << active_cores << " limited to hardware maximum cores "
                              << hardware_cores << std::endl;
                    active_cores = std::min(active_cores, hardware_cores);
                }
            }
        }

        assert(!args.schedule.empty());
        std::cout << "Schedule: ";
        for (std::size_t i{0}; i < args.schedule.size() - 1; ++i) {
            std::cout << args.schedule[i] << ",";
        }
        std::cout << args.schedule.back() << std::endl;
        std::cout << std::endl;
    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << std::endl;
        std::cerr << program;
        exit(1);
    }
    return args;
}

// 自动调参阶段依赖
void Burn(std::size_t iter) {
    // LCG like payload
    constexpr std::uint64_t a{6364136223846793005};
    constexpr std::uint64_t c{1442695040888963407};
    volatile std::size_t x{0};
    for (std::size_t i{0}; i < iter; ++i) {
        x = a * x + c;
    }
}

std::size_t AutoTune(std::chrono::steady_clock::duration target, std::size_t init_iter) {
    constexpr std::size_t MAX_TUNE_NUM{100}; // 最大调节次数
    constexpr double MAX_RATIO{10.};         // 最大调节倍数：1 / MAX_RATIO <= 单次调节倍数 <= MAX_RATIO
    constexpr double ALPHA{.3};              // 平滑因子：0 < ALPHA < 1，越小越稳定，越大响应越快
    static_assert(0 < ALPHA && ALPHA < 1);

    std::cout << "Target: " << 1000 * std::chrono::duration<double>{target}.count() << " ms" << std::endl;
    std::cout << "Initial iter: " << init_iter << std::endl;

    std::cout << std::setw(4) << "Step";
    std::cout << std::setw(10) << "Iter";
    std::cout << std::setw(10) << "Actual";
    std::cout << std::setw(8) << "RErr" << std::endl;

    std::size_t iter{init_iter};
    std::size_t tune_num{0};
    for (; tune_num < MAX_TUNE_NUM; ++tune_num) {
        auto start{std::chrono::steady_clock::now()};
        Burn(iter);
        auto end{std::chrono::steady_clock::now()};

        double actual_s{std::chrono::duration<double>(end - start).count()};
        double target_s{std::chrono::duration<double>(target).count()};
        double err_pct{std::abs(100 * (actual_s - target_s) / target_s)};

        // 参数打印
        std::cout << std::setw(4) << tune_num;
        std::cout << std::setw(10) << iter;

        std::ostringstream actual_ms_oss;
        actual_ms_oss << std::fixed << std::setprecision(2) << actual_s * 1000;
        std::cout << std::setw(10) << actual_ms_oss.str() + " ms";

        std::ostringstream err_pct_oss;
        err_pct_oss << std::fixed << std::setprecision(2) << err_pct;
        std::cout << std::setw(8) << err_pct_oss.str() + '%' << std::endl;

        if (err_pct < 0.1) {
            break;
        }

        double ratio{target_s / actual_s};
        ratio = std::min(ratio, MAX_RATIO);
        ratio = std::max(ratio, 1 / MAX_RATIO);

        double new_iter{iter * ratio};
        // EMA平滑
        iter = static_cast<double>(iter) * (1.0 - ALPHA) + new_iter * ALPHA;
    }

    if (tune_num >= MAX_TUNE_NUM) {
        std::cout << "Tune count exceeds maximum limit " << MAX_TUNE_NUM << std::endl;
    }
    std::cout << std::endl;
    return iter;
}

std::size_t AutoTune(std::chrono::steady_clock::duration target) {
    constexpr std::size_t ITER_PER_US{400}; // 经验初值
    auto target_us{std::chrono::duration_cast<std::chrono::microseconds>(target).count()};
    return AutoTune(target, target_us * ITER_PER_US);
}

int main(int argc, char *argv[]) {
    std::cout << "Pid: " << getpid() << std::endl;
    const Args args{ParseArgs(argc, argv)};

    // 自动调节迭代次数，单次Burn耗时约为itv的1%
    std::cout << "Auto-tuning burn iter num..." << std::endl;
    const std::size_t burn_iter{AutoTune(std::chrono::duration_cast<std::chrono::microseconds>(args.itv) / 100)};

    std::atomic<bool> stop{false};
    const auto start_time{std::chrono::steady_clock::now()};

    auto load = [&args, burn_iter, &stop, start_time](int thread_seq) {
        auto time{std::chrono::steady_clock::now()};
        auto step{(time - start_time) / args.itv};
        auto next_time{start_time + (step + 1) * args.itv};
        while (!stop) {
            int active_cores{args.schedule[step % args.schedule.size()]};
            if (thread_seq < active_cores) {
                while (std::chrono::steady_clock::now() < next_time) {
                    Burn(burn_iter);
                }
            } else {
                std::this_thread::sleep_until(next_time);
            }
            ++step;
            next_time += args.itv;
        }
    };

    std::cout << "Starting load execution..." << std::endl;
    const int max_active_cores{*std::max_element(args.schedule.begin(), args.schedule.end())};
    std::vector<std::thread> workers;
    for (int i{0}; i < max_active_cores; ++i) {
        workers.emplace_back(load, i);
    }

    auto time{std::chrono::steady_clock::now()};
    oops::CpuTimer cpu_timer;
    auto step{(time - start_time) / args.itv};
    auto next_time{start_time + (step + 1) * args.itv};

    std::cout << std::setw(4) << "Step";
    std::cout << std::setw(6) << "Cores";
    std::cout << std::setw(10) << "ActUsage" << std::endl;

    while (!args.step_num.has_value() || step < *args.step_num) {
        int active_cores{args.schedule[step % args.schedule.size()]};
        std::cout << std::setw(4) << step;
        std::cout << std::setw(6) << active_cores;
        std::cout.flush();

        std::this_thread::sleep_until(next_time);
        auto cpu_usage_pct{cpu_timer.Lap().CpuUsagePct()};
        std::ostringstream actual_usage_oss;
        actual_usage_oss << std::fixed << std::setprecision(2) << cpu_usage_pct;
        std::cout << std::setw(10) << actual_usage_oss.str() + '%' << std::endl;

        ++step;
        next_time += args.itv;
    }

    stop = true;
    for (auto &w : workers) {
        w.join();
    }
    return 0;
}
