#include <cassert>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>

#include <sys/stat.h>
#include <unistd.h>

#include "oops/cpu_timer.h"

std::string Timestamp(std::chrono::system_clock::time_point time_point) {
    std::time_t time{std::chrono::system_clock::to_time_t(time_point)};
    std::tm *tm{std::localtime(&time)}; // std::localtime不可重入

    auto frac{time_point - std::chrono::time_point_cast<std::chrono::seconds>(time_point)};
    auto frac_us{std::chrono::duration_cast<std::chrono::microseconds>(frac).count()};

    std::ostringstream oss;
    oss << std::put_time(tm, "%Y-%m-%d %H:%M:%S") << '.' << std::setfill('0') << std::setw(6) << frac_us;
    return oss.str();
}

bool ProcExist(pid_t pid) {
    static std::string path{"/proc/" + std::to_string(pid)};
    struct stat buffer;
    return stat(path.c_str(), &buffer) == 0;
}

int main(int, char *argv[]) {
    pid_t pid{std::stoi(argv[1])};
    std::chrono::milliseconds itv{100};
    std::cout << "Tracked pid: " << pid << std::endl;
    std::cout << "Interval: " << itv.count() << " ms" << std::endl;
    std::cout << "Ticks per sec: " << oops::GetTicksPerSec() << std::endl;

    // 系统时钟盖时间戳，单调时钟算时间差
    auto system_now{std::chrono::system_clock::now()};
    oops::CpuTimer cpu_timer(pid);
    std::cout << "Timestamp: " << Timestamp(system_now) << '\n' << std::endl;
    std::size_t step{0};
    auto next_time{cpu_timer.GetElapsedT0() + (step + 1) * itv};

    while (ProcExist(pid)) {
        std::this_thread::sleep_until(next_time);
        auto cpu_usage{cpu_timer.Lap().CpuUsage()};
        std::cout << cpu_usage << std::endl;

        ++step;
        next_time += itv;
    }
    return 0;
}
