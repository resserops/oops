#include <array>
#include <cassert>
#include <charconv>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>
#include <tuple>
#include <vector>

#include <sys/stat.h>
#include <unistd.h>

#include "argparse/argparse.hpp"
#include "glob/glob.h"

#include "oops/cpu_timer.h"
#include "oops/enum_bitset.h"
#include "oops/str.h"
#include "oops/system_info.h"
#include "oops/unit.h"

namespace fs = std::filesystem;

// 配置全局变量
enum class ProgramType : uint8_t { MEASURE, REPORT };

inline struct Args {
    ProgramType program_type{};

    struct {
        pid_t pid{};
        double itv{};
    } measure;

    struct {
        std::vector<std::string> input; // 可能是glob pattern
        std::vector<fs::path> dir;
        // 根据dir和input形成最终的文件列表
        std::vector<fs::path> valid_input;
    } report;
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
inline MetricEntry METRIC_TABLE[] = {
    {"Cpu(%)", 6, MetricGroup::CPU},
    {"Rss(G)", 6, MetricGroup::MEMORY},
    {"Hwm(G)", 6, MetricGroup::MEMORY},
    {"Swap(G)", 6, MetricGroup::MEMORY}};

// 公共函数
std::string TimestampToStr(std::chrono::system_clock::time_point time_point) {
    std::time_t time{std::chrono::system_clock::to_time_t(time_point)};
    std::tm *tm{std::localtime(&time)}; // std::localtime不可重入

    auto frac{time_point - std::chrono::time_point_cast<std::chrono::seconds>(time_point)};
    auto frac_us{std::chrono::duration_cast<std::chrono::microseconds>(frac).count()};

    std::ostringstream oss;
    oss << std::put_time(tm, "%Y-%m-%d %H:%M:%S") << '.' << std::setfill('0') << std::setw(6) << frac_us;
    return oss.str();
}

std::chrono::system_clock::time_point StrToTimestamp(std::string_view sv) {
    if (sv.size() < 26) {
        throw std::runtime_error("size not match");
    }
    std::tm tm{};

    // 提取"YYYY-mm-dd HH:MM:SS"
    std::istringstream iss{std::string(sv.substr(0, 19))};
    iss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
    auto time_point{std::chrono::system_clock::from_time_t(std::mktime(&tm))};

    // 解析微秒
    std::size_t us;
    std::from_chars(sv.data() + 20, sv.data() + 26, us);
    return time_point + std::chrono::microseconds(us);
}

bool ProcExist(pid_t pid) {
    static std::string path{"/proc/" + std::to_string(pid)};
    struct stat buffer;
    return stat(path.c_str(), &buffer) == 0;
}

struct SampleEntry {
    SampleEntry(std::string s) : name{std::move(s)} {}
    SampleEntry(std::string_view s) : name{s} {}
    std::string name;
    std::vector<double> values;
};

struct RawDataEntry {
    fs::path raw_file_path;
    pid_t pid{};
    std::size_t interval{};
    std::chrono::system_clock::time_point start_time;
    std::vector<SampleEntry> sample_table;
};

inline std::vector<RawDataEntry> RAW_DATA_TABLE;

struct ParseValueAfterPrefixResult {
    explicit operator bool() const noexcept { return matched && parsed; }
    std::string_view remain;
    bool matched{false};
    bool parsed{false};
};

template <typename T>
auto ParseValueAfterPrefix(std::string_view s, std::string_view prefix, T &value) noexcept {
    ParseValueAfterPrefixResult res;
    s = oops::Strip(s);

    auto pos{s.find(prefix)};
    if (pos == std::string_view::npos) {
        return res;
    }
    s.remove_prefix(pos + prefix.size());
    s = oops::Strip(s); // 使用StripLeft优化
    res.matched = true;
    res.remain = s;

    auto [ptr, errc]{std::from_chars(s.data(), s.data() + s.size(), value)};
    if (errc != std::errc{}) {
        return res;
    }
    s.remove_prefix(ptr - s.data());
    res.parsed = true;
    res.remain = oops::Strip(s);
    return res;
}

void ParseRawData(std::istream &is, RawDataEntry &entry) {
    using namespace std::literals;

    // 解析阶段控制变量
    enum class Step { META = 0, HEADER_ROW, DATA_ROW } step{};
    std::size_t metric_num{};

    std::string buf;
    while (std::getline(is, buf)) {
        std::string_view line{buf};
        line = oops::Strip(line);
        if (line.empty()) {
            continue;
        }

        switch (step) {
        case Step::META: {
            if (ParseValueAfterPrefix(line, "Tracked pid:"sv, entry.pid)) {
                break;
            }
            if (ParseValueAfterPrefix(line, "Interval:"sv, entry.interval)) {
                break;
            }
            std::size_t pos{line.find("Timestamp:")}; // 提取解析函数优化
            if (pos != std::string_view::npos) {
                entry.start_time = StrToTimestamp(line.substr(pos + 10));
                break;
            }
            if (line.find("Monitoring results:") != std::string_view::npos) {
                step = Step::HEADER_ROW;
            }
            break;
        }

        case Step::HEADER_ROW: {
            auto tokens{oops::Split<std::vector<std::string_view>>(line)};
            metric_num = tokens.size();
            for (auto token : tokens) {
                entry.sample_table.emplace_back(token);
            }
            step = Step::DATA_ROW;
            break;
        }

        case Step::DATA_ROW: {
            auto tokens{oops::Split<std::vector<std::string_view>>(line)};
            if (metric_num == 0) {
                throw std::runtime_error("bad metric num");
            }
            if (metric_num != tokens.size()) {
                throw std::runtime_error("bad metric num");
            }
            for (std::size_t i{0}; i < tokens.size(); ++i) {
                double d{};
                std::from_chars(tokens[i].data(), tokens[i].data() + tokens[i].size(), d);
                entry.sample_table[i].values.push_back(d);
            }
            break;
        }
        }
    }
}

void ParseRawData(const fs::path &path, RawDataEntry &entry) {
    std::ifstream ifs(path);
    ParseRawData(ifs, entry);
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

void Measure() {
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "Tracked pid: " << ARGS.measure.pid << std::endl;
    std::cout << "Interval: " << ARGS.measure.itv << "s" << std::endl;
    std::cout << "Ticks per sec: " << oops::GetTicksPerSec() << std::endl;

    // itv转换为毫秒定点数
    std::chrono::milliseconds itv{static_cast<std::size_t>(1000 * ARGS.measure.itv)};

    // 系统时钟盖时间戳，单调时钟算时间差
    auto system_now{std::chrono::system_clock::now()};
    oops::CpuTimer cpu_timer(ARGS.measure.pid); // 用于测量cpu时间

    std::cout << "Timestamp: " << TimestampToStr(system_now) << '\n' << std::endl;
    std::size_t step{0};
    auto next_time{cpu_timer.GetElapsedT0() + (step + 1) * itv};

    std::cout << "Monitoring results:" << std::endl;
    std::cout << MakeHeaderRow() << std::endl;

    std::vector<double> values(oops::ToUnderlying(Metrics::COUNT));
    while (ProcExist(ARGS.measure.pid)) {
        std::this_thread::sleep_until(next_time);

        // 根据选项配置完成测量
        if (ENABLED_METRIC_GROUP.Test(MetricGroup::CPU)) { // 有限测量区间指标，再测量单点指标
            values[oops::ToUnderlying(Metrics::CPU_USAGE)] = cpu_timer.Lap().CpuUsage();
        }

        if (ENABLED_METRIC_GROUP.Test(MetricGroup::MEMORY)) {
            using namespace oops::proc::status;
            Info info{Get(ARGS.measure.pid, Field::VM_RSS | Field::VM_HWM | Field::VM_SWAP)};
            values[oops::ToUnderlying(Metrics::RSS)] = oops::GiBs<double>{oops::KiBs<>{*info.vm_rss}}.Count();
            values[oops::ToUnderlying(Metrics::HWM)] = oops::GiBs<double>{oops::KiBs<>{*info.vm_hwm}}.Count();
            values[oops::ToUnderlying(Metrics::SWAP)] = oops::GiBs<double>{oops::KiBs<>{*info.vm_swap}}.Count();
        }

        // 打印测量结果，解耦测量和打印，支撑后续二进制格式
        std::cout << MakeDataRow(values) << std::endl;

        ++step;
        next_time += itv;
    }
}

void SearchValidInput() {
    std::vector<fs::path> literal_input;
    std::vector<std::string> patterns;
    for (const auto &input : ARGS.report.input) {
        fs::path input_path{input};
        if (input_path.is_absolute()) {
            if (input.find_first_of("*?") != std::string::npos) {
                // glob绝对路径
                patterns.push_back(input);
            } else {
                // 普通绝对路径
                literal_input.push_back(input_path);
            }
        } else {
            assert(input_path.is_relative());
            for (const auto &dir : ARGS.report.dir) {
                assert(!fs::is_directory(dir));
                if (input.find_first_of("*?") != std::string::npos) {
                    // glob相对路径，那么在dir中递归查找glob
                    patterns.push_back((dir / "**" / input_path).string());
                } else {
                    // 普通相对路径，仅拼接
                    literal_input.push_back(dir / input_path);
                }
            }
        }
    }

    // glob匹配
    for (const auto &pattern : patterns) {
        for (const auto &path : glob::rglob(pattern)) {
            literal_input.push_back(path);
        }
    }

    // 经典化路径并去重
    std::vector<fs::path> valid_input;
    for (const auto &path : literal_input) {
        std::error_code errc;
        if (fs::exists(path, errc) && fs::is_regular_file(path, errc)) {
            fs::path canonical_path{fs::canonical(path)};
            if (!errc) {
                valid_input.push_back(std::move(canonical_path));
            } else {
                std::cerr << "Warning: bad path. ignored" << std::endl;
            }
        }
    }

    // 去重
    std::sort(valid_input.begin(), valid_input.end());
    auto pos{std::unique(valid_input.begin(), valid_input.end())};
    valid_input.erase(pos, valid_input.end());

    ARGS.report.valid_input = std::move(valid_input);
}

void Report() {
    SearchValidInput();
    std::size_t raw_file_num{ARGS.report.valid_input.size()};
    RAW_DATA_TABLE.resize(raw_file_num);
    for (std::size_t i{0}; i < raw_file_num; ++i) {
        ParseRawData(ARGS.report.valid_input[i], RAW_DATA_TABLE[i]);
    }
}

// 参数解析
void ParseArgs(int argc, char *argv[]) {
    argparse::ArgumentParser program{"monitor", "1.0"};

    argparse::ArgumentParser measure{"measure"};
    measure.add_argument("pid").help("target process id to monitor").scan<'i', pid_t>().required();
    measure.add_argument("-i", "--itv")
        .help("sampling interval in secondes (e.g., 0.5 for 500ms)")
        .default_value(1.)
        .scan<'g', double>();
    measure.add_argument("-m", "--metric")
        .help("comma-separated sequence of metrics to monitor (e.g., cpu[c],memory[m])")
        .default_value("all");
    program.add_subparser(measure);

    argparse::ArgumentParser report{"report"};
    report.add_description("process and visualize monitor raw data");
    report.add_argument("input").help("comma-separated sequence of raw data file").required();
    report.add_argument("-d", "--dir")
        .help("the root directory to search for files matching the 'input' pattern")
        .default_value(std::filesystem::current_path().string());
    program.add_subparser(report);

    try {
        program.parse_args(argc, argv);
    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << std::endl;
        std::cerr << program;
        exit(1);
    }

    if (program.is_subcommand_used("measure")) {
        // program type
        ARGS.program_type = ProgramType::MEASURE;

        // pid
        ARGS.measure.pid = measure.get<pid_t>("pid");

        // --itv
        ARGS.measure.itv = measure.get<double>("--itv");
        if (ARGS.measure.itv < .01) {
            std::cerr << "Error: interval must be greater than 10ms" << std::endl;
            exit(1);
        }

        // --metric
        std::string metric{measure.get<std::string>("--metric")};
        auto metric_tokens{oops::Split<std::vector<std::string_view>>(metric, ',')};

        for (auto token : metric_tokens) {
            bool matched{false};
            if ((token == "all" || token == "cpu" || token == "c") && !ENABLED_METRIC_GROUP.Test(MetricGroup::CPU)) {
                ENABLED_METRIC_GROUP.Set(MetricGroup::CPU);
                matched = true;
            }
            if ((token == "all" || token == "memory" || token == "m") &&
                !ENABLED_METRIC_GROUP.Test(MetricGroup::MEMORY)) {
                ENABLED_METRIC_GROUP.Set(MetricGroup::MEMORY);
                matched = true;
            }
            if (!matched) {
                std::cerr << "Warning: ignore unexpected metric " << token << std::endl;
            }
        }
        if (ENABLED_METRIC_GROUP.none()) {
            std::cerr << "Error: no metrics available" << std::endl;
            exit(1);
        }
    } else if (program.is_subcommand_used("report")) {
        // program type
        ARGS.program_type = ProgramType::REPORT;

        // input
        std::string input{report.get<std::string>("input")};
        auto input_tokens{oops::Split<std::vector<std::string_view>>(input, ',')};
        for (auto token : input_tokens) {
            ARGS.report.input.emplace_back(token);
        }

        // --dir
        std::string dir{report.get<std::string>("--dir")};
        auto tokens{oops::Split<std::vector<std::string_view>>(dir, ',')};
        for (auto token : tokens) {
            fs::path dir_path{token};
            if (!fs::is_directory(dir_path)) {
                std::cerr << "Warning: \"" << token << "\" in --dir is not a directory. ignored" << std::endl;
                continue;
            }
            ARGS.report.dir.emplace_back(token);
        }
        if (ARGS.report.dir.empty()) {
            std::cerr << "Warning: no valid directories provided in --dir. using default current directory"
                      << std::endl;
            ARGS.report.dir.emplace_back(fs::current_path());
        }
    } else {
        std::cout << program << std::endl;
    }
}

int main(int argc, char *argv[]) {
    ParseArgs(argc, argv);
    if (ARGS.program_type == ProgramType::MEASURE) {
        Measure();
    } else {
        Report();
    }
    return 0;
}
