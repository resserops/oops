#include "oops/cpu_freq.h"

#include <algorithm>
#include <charconv>
#include <fstream>
#include <numeric>
#include <string>
#include <string_view>
#include <system_error>

#include "oops/proc/cpuinfo.h"
#include "oops/str.h"

namespace oops {
namespace cpu_freq {
namespace {
auto ParseRangeList(std::string_view s) {
    std::vector<std::size_t> res;
    for (auto token : Split(s, ',')) {
        std::size_t l{};
        const auto token_end{token.data() + token.size()};
        auto lr{std::from_chars(token.data(), token_end, l)};
        if (lr.ec != std::errc{}) {
            continue;
        }
        if (lr.ptr == token_end) {
            res.push_back(l);
            continue;
        }
        if (*lr.ptr != '-') {
            continue;
        }

        std::size_t r{};
        auto rr{std::from_chars(lr.ptr + 1, token_end, r)};
        if (rr.ec != std::errc{}) {
            continue;
        }
        if (rr.ptr != token_end) {
            continue;
        }
        for (std::size_t i{l}; i <= r; ++i) {
            res.push_back(i);
        }
    }
    return res;
}

std::vector<std::size_t> GetOnlineCpus() {
    std::ifstream ifs{"/sys/devices/system/cpu/online"};
    std::string buf;
    std::getline(ifs, buf);
    return ParseRangeList(buf);
}

Info GetFromSysfs() {
    Info info{Source::SYSFS, {}};
    const auto online_cpus{GetOnlineCpus()};
    info.cpus.reserve(online_cpus.size());
    for (std::size_t cpu : online_cpus) {
        std::ifstream ifs{"/sys/devices/system/cpu/cpu" + std::to_string(cpu) + "/cpufreq/scaling_cur_freq"};
        double khz{};
        if (ifs >> khz) {
            info.cpus.push_back({cpu, khz / 1000.});
        }
    }
    return info;
}

Info GetFromCpuinfo() {
    namespace cpuinfo = oops::proc::cpuinfo;

    Info info{Source::CPUINFO, {}};
    auto cpu_info{cpuinfo::Get(cpuinfo::Field::CPU_MHZ | cpuinfo::Field::PROCESSOR)};
    info.cpus.reserve(cpu_info.table.size());
    for (const auto &cpu : cpu_info.table) {
        if (cpu.parsed.Test(cpuinfo::Field::CPU_MHZ)) {
            info.cpus.push_back({cpu.processor, cpu.cpu_mhz});
        }
    }
    return info;
}
} // namespace

Info Get() {
    static const bool USE_SYSFS{!GetFromSysfs().cpus.empty()};
    if (USE_SYSFS) {
        return GetFromSysfs();
    }
    static const bool USE_CPUINFO{!GetFromCpuinfo().cpus.empty()};
    if (USE_CPUINFO) {
        return GetFromCpuinfo();
    }
    return Info{};
}

double Info::Average() const {
    if (cpus.empty()) {
        return 0.;
    }
    auto op{[](double acc, const Entry &entry) { return acc + entry.mhz; }};
    const double sum{std::accumulate(cpus.begin(), cpus.end(), 0., op)};
    return sum / cpus.size();
}

double Info::Max() const {
    if (cpus.empty()) {
        return 0.;
    }
    auto op{[](const Entry &lhs, const Entry &rhs) { return lhs.mhz < rhs.mhz; }};
    return std::max_element(cpus.begin(), cpus.end(), op)->mhz;
}

double Info::Min() const {
    if (cpus.empty()) {
        return 0.;
    }
    auto op{[](const Entry &lhs, const Entry &rhs) { return lhs.mhz < rhs.mhz; }};
    return std::min_element(cpus.begin(), cpus.end(), op)->mhz;
}
} // namespace cpu_freq
} // namespace oops
