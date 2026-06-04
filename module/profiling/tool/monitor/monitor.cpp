#include <array>
#include <cassert>
#include <charconv>
#include <chrono>
#include <csignal>
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
#include "glob/glob.hpp"
#include "pybind11/embed.h"
#include "pybind11/stl.h"

#include "oops/cpu_timer.h"
#include "oops/enum_bitset.h"
#include "oops/str.h"
#include "oops/system_info.h"
#include "oops/unit.h"

namespace fs = std::filesystem;
namespace py = pybind11;

// 配置全局变量
enum class ProgramType : uint8_t { MEASURE, REPORT };

inline struct Args {
    ProgramType program_type{};

    struct {
        pid_t pid{};
        double itv{};
    } measure{};

    struct {
        std::vector<std::string> input;    // 可能是glob pattern
        std::vector<fs::path> dir;         // 搜索glob pattern的根目录
        std::vector<fs::path> valid_input; // 根据dir和input形成最终的文件列表

        std::size_t resolution{}; // 横轴显示几个时间点
        struct {
            // 从TOTAL_START_TIME开始的duration
            std::chrono::system_clock::duration start{0};
            std::chrono::system_clock::duration end{std::chrono::system_clock::duration::max()};
        } range{};

        bool show{};
    } report{};
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
    std::cout << "Tracked pid: " << ARGS.measure.pid << std::endl;
    std::cout << "Interval: " << ARGS.measure.itv << "s" << std::endl;
    std::cout << "Ticks per sec: " << oops::GetTicksPerSec() << std::endl;

    // itv转换为毫秒定点数
    std::chrono::milliseconds itv{static_cast<std::size_t>(1000 * ARGS.measure.itv)};

    // 注册信号
    RegisterSignalHandler();

    // 系统时钟盖时间戳，单调时钟算时间差
    auto system_now{std::chrono::system_clock::now()};
    oops::CpuTimer cpu_timer(ARGS.measure.pid); // 用于测量cpu时间

    std::cout << "Timestamp: " << TimestampToStr(system_now) << '\n' << std::endl;
    std::size_t step{0};
    auto next_time{cpu_timer.GetElapsedT0() + (step + 1) * itv};

    std::cout << "Monitoring results:" << std::endl;
    std::cout << MakeHeaderRow() << std::endl;

    std::vector<double> values(oops::ToUnderlying(Metrics::COUNT));
    while (!STOP.load(std::memory_order_relaxed) && ProcExist(ARGS.measure.pid)) {
        std::this_thread::sleep_until(next_time);

        // 根据选项配置完成测量
        if (ENABLED_METRIC_GROUP.Test(MetricGroup::CPU)) { // 有限测量区间指标，再测量单点指标
            values[oops::ToUnderlying(Metrics::CPU_USAGE)] = 100 * cpu_timer.Lap().CpuUsage();
        }

        if (ENABLED_METRIC_GROUP.Test(MetricGroup::MEMORY)) {
            using namespace oops::proc::status;
            Info info{Get(ARGS.measure.pid, Field::VM_RSS | Field::VM_HWM | Field::VM_SWAP)};
            values[oops::ToUnderlying(Metrics::RSS)] = oops::GiBs<double>{oops::KiBs<>{info.vm_rss}}.Count();
            values[oops::ToUnderlying(Metrics::HWM)] = oops::GiBs<double>{oops::KiBs<>{info.vm_hwm}}.Count();
            values[oops::ToUnderlying(Metrics::SWAP)] = oops::GiBs<double>{oops::KiBs<>{info.vm_swap}}.Count();
        }

        // 打印测量结果，解耦测量和打印，支撑后续二进制格式
        std::cout << MakeDataRow(values) << std::endl;

        ++step;
        next_time += itv;
    }
}

// 报告全局变量
struct MetricDataEntry {
    MetricDataEntry(std::string s) : name{std::move(s)} {}
    MetricDataEntry(std::string_view s) : name{s} {}
    std::string name;
    std::vector<double> values;
};

struct RawDataEntry {
    RawDataEntry(std::string s) : raw_file_name{std::move(s)} {}
    std::size_t MetricDataSize() const {
        if (metric_data_table.empty()) {
            return 0;
        }
        return metric_data_table.front().values.size();
    }

    auto EndTime() const {
        return start_time + std::chrono::duration_cast<std::chrono::system_clock::duration>(
                                std::chrono::duration<double>{MetricDataSize() * interval});
    }

    auto Duration() const { return EndTime() - start_time; }

    std::string raw_file_name;
    pid_t pid{};
    double interval{}; // TODO(resserops): 必须大于0，parse时增加校验
    std::chrono::system_clock::time_point start_time;
    std::vector<MetricDataEntry> metric_data_table;

    // 计算得到
    std::size_t downsample_rate{}; // 将原始数据的N个点合并为1个点
};

inline std::chrono::system_clock::time_point TOTAL_START_TIME{};
inline std::vector<RawDataEntry> RAW_DATA_TABLE;

// 报告功能
std::chrono::system_clock::time_point StrToTimestamp(std::string_view sv) {
    sv = oops::Strip(sv);
    if (sv.size() < 26) {
        throw std::runtime_error("size too small for timestamp format");
    }
    std::tm tm{};

    // 提取"YYYY-mm-dd HH:MM:SS"
    std::istringstream iss{std::string(sv.substr(0, 19))};
    iss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
    auto time_point{std::chrono::system_clock::from_time_t(std::mktime(&tm))};

    // 解析微秒
    std::size_t us{};
    std::from_chars(sv.data() + 20, sv.data() + 26, us);
    return time_point + std::chrono::microseconds(us);
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
                assert(fs::is_directory(dir));
                if (input.find_first_of("*?") != std::string::npos) {
                    // glob相对路径，那么在dir中递归查找glob
                    patterns.push_back((dir / "**" / input_path).string());
                    patterns.push_back((dir / input_path).string());
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

template <typename T>
auto ParseValueAfterPrefix(std::string_view s, std::string_view prefix, T &value) noexcept {
    struct {
        explicit operator bool() const noexcept { return matched && parsed; }
        std::string_view remain;
        bool matched{false};
        bool parsed{false};
    } res;

    s = oops::Strip(s);
    auto pos{s.find(prefix)};
    if (pos == std::string_view::npos) {
        return res;
    }
    s.remove_prefix(pos + prefix.size());
    s = oops::StripLeft(s);
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
            auto tokens{oops::Split(line, ", ")};
            for (auto token : tokens) {
                entry.metric_data_table.emplace_back(oops::Strip(token));
            }
            metric_num = entry.metric_data_table.size();
            step = Step::DATA_ROW;
            break;
        }

        case Step::DATA_ROW: {
            auto tokens{oops::Split(line, ", ").To<std::vector>()};
            if (metric_num == 0) {
                throw std::runtime_error("bad metric num");
            }
            if (metric_num != tokens.size()) {
                throw std::runtime_error("bad metric num");
            }
            for (std::size_t i{0}; i < tokens.size(); ++i) {
                double d{};
                auto token{oops::Strip(tokens[i])};
                std::from_chars(token.data(), token.data() + token.size(), d);
                entry.metric_data_table[i].values.push_back(d);
            }
            break;
        }
        }
    }
    assert(step == Step::DATA_ROW);
}

void ParseRawData(const fs::path &path, RawDataEntry &entry) {
    std::ifstream ifs(path);
    ParseRawData(ifs, entry);
}

void AlignTimeline() {
    // 计算所有raw data的公共start time
    auto f_min_start = [](const auto &lhs, const auto &rhs) { return lhs.start_time < rhs.start_time; };
    TOTAL_START_TIME = std::min_element(RAW_DATA_TABLE.begin(), RAW_DATA_TABLE.end(), f_min_start)->start_time;

    // 计算timeline range截断
    for (auto &r_entry : RAW_DATA_TABLE) {
        assert(r_entry.interval > .0);
        std::size_t size{r_entry.MetricDataSize()};
        if (size == 0) {
            continue;
        }

        // 计算从TOTAL_START_TIME到当前raw data左右边界的duration
        auto raw_l{r_entry.start_time - TOTAL_START_TIME};
        auto raw_r{r_entry.EndTime() - TOTAL_START_TIME};
        assert(raw_l.count() >= 0);
        assert(raw_r >= raw_l);

        // 与range取交集，TOTAL_START_TIME到range左右边界的duration
        auto range_l{std::max(raw_l, ARGS.report.range.start)};
        auto range_r{std::min(raw_r, ARGS.report.range.end)};
        assert(range_l.count() >= 0);
        assert(range_r >= range_l);

        // 减得raw data到range左右边界的duration，计算range在metric data中的左右下标
        double range_l_fidx{std::ceil(std::chrono::duration<double>{range_l - raw_l}.count()) / r_entry.interval};
        double range_r_fidx{std::ceil(std::chrono::duration<double>{range_r - raw_l}.count()) / r_entry.interval};

        std::cout << "lidx: " << range_l_fidx << std::endl;
        std::cout << "ridx: " << range_r_fidx << std::endl;

        // 计算range对应的下标，左闭右开
        std::size_t range_l_idx{std::max<std::ptrdiff_t>(range_l_fidx, 0)};
        std::size_t range_r_idx{std::min<std::ptrdiff_t>(range_r_fidx, size)};
        assert(range_l_idx <= range_r_idx);

        // range切片
        r_entry.start_time += std::chrono::duration_cast<std::chrono::system_clock::duration>(
            std::chrono::duration<double>{range_l_idx * r_entry.interval});
        for (auto &metric : r_entry.metric_data_table) {
            if (range_l_idx > 0) {
                metric.values.erase(metric.values.begin(), metric.values.begin() + range_l_idx);
            }
            metric.values.resize(range_r_idx - range_l_idx);
        }
    }

    // 重新计算range切片后的公共start time和end time，获取数据区间总长，用于计算向下采样倍率
    auto total_range_start_time{
        std::min_element(RAW_DATA_TABLE.begin(), RAW_DATA_TABLE.end(), f_min_start)->start_time};
    auto f_max_end = [](const auto &lhs, const auto &rhs) { return lhs.EndTime() < rhs.EndTime(); };
    auto total_range_end_time{std::max_element(RAW_DATA_TABLE.begin(), RAW_DATA_TABLE.end(), f_max_end)->EndTime()};
    auto total_range_duration{total_range_end_time - total_range_start_time};

    // 根据时间轴分辨率，计算每份raw data的向下采样倍率
    for (auto &r_entry : RAW_DATA_TABLE) {
        // 每downsample_rate个点合并为1个点，如何合并由每项指标自己定义
        std::size_t range_resolution{
            ARGS.report.resolution * r_entry.Duration().count() / total_range_duration.count()};
        assert(range_resolution > 0);
        r_entry.downsample_rate = r_entry.MetricDataSize() / range_resolution;
    }
}

void PlotCpu() {
    if (RAW_DATA_TABLE.empty()) {
        return;
    }

    // 识别记录了CPU利用率的raw data idx及对应CPU利用率的metric data dix
    std::vector<std::pair<std::size_t, std::size_t>> valid_idxs;
    for (std::size_t i{0}; i < RAW_DATA_TABLE.size(); ++i) {
        const auto &r_entry{RAW_DATA_TABLE[i]};
        for (std::size_t j{0}; j < r_entry.metric_data_table.size(); ++j) {
            const auto &m_entry{r_entry.metric_data_table.at(j)};
            if (m_entry.name == METRIC_TABLE[oops::ToUnderlying(Metrics::CPU_USAGE)].name && !m_entry.values.empty()) {
                valid_idxs.emplace_back(i, j);
            }
        }
    }
    if (valid_idxs.empty()) {
        return;
    }

    // 根据downsample_rate，合并数据点，cpu利用率采取平均值
    for (const auto &[r_idx, m_idx] : valid_idxs) {
        auto &r_entry{RAW_DATA_TABLE[r_idx]};
        if (r_entry.downsample_rate <= 1) {
            // 不需要合并
            continue;
        }

        std::cout << r_entry.downsample_rate << std::endl;

        // 计算合并后的新数据点数
        auto &m_entry{r_entry.metric_data_table[m_idx]};
        std::size_t new_i{0};
        for (std::size_t i{0}; i + r_entry.downsample_rate - 1 < m_entry.values.size(); i += r_entry.downsample_rate) {
            double sum{};
            for (std::size_t j{i}; j < i + r_entry.downsample_rate; j++) {
                sum += m_entry.values[j];
            }
            m_entry.values[new_i++] = sum / r_entry.downsample_rate;
            std::cout << "avg: " << sum / r_entry.downsample_rate << std::endl;
        }
        m_entry.values.resize(new_i);
        r_entry.interval *= r_entry.downsample_rate;
    }

    // 最大数值
    double max_value{std::numeric_limits<double>::min()};
    for (const auto &[r_idx, m_idx] : valid_idxs) {
        for (double value : RAW_DATA_TABLE[r_idx].metric_data_table[m_idx].values) {
            max_value = std::max(max_value, value);
        }
    }
    std::cout << "max_value: " << max_value << std::endl;

    py::scoped_interpreter guard{}; // 必须放在try-catch块外侧，否则无法捕获py::error_already_set异常
    try {
        auto plt{py::module_::import("matplotlib.pyplot")};
        auto subplots{plt.attr("subplots")(
            valid_idxs.size(), 1, py::arg("sharex") = true,
            py::arg("figsize") = py::make_tuple(
                16, std::max<std::size_t>(std::min<std::size_t>(9, 3 * valid_idxs.size()), valid_idxs.size())))};

        py::list axes; // 存储所有子图，使用vector反而会有拷贝和引用计数开销
        if (valid_idxs.size() == 1) {
            axes.append(subplots[py::int_(1)]);
        } else {
            axes = subplots[py::int_(1)].cast<py::list>();
        }

        for (std::size_t i{0}; i < valid_idxs.size(); ++i) {
            const auto &r_entry{RAW_DATA_TABLE[valid_idxs[i].first]};
            const auto &m_entry{r_entry.metric_data_table[valid_idxs[i].second]};
            double offset{std::chrono::duration<double>{r_entry.start_time - TOTAL_START_TIME}.count()};

            // 构造时间数组，由于offset差异，每个raw data需要单独构造
            std::vector<double> times;
            for (std::size_t j{0}; j < m_entry.values.size(); ++j) {
                times.push_back(offset + j * r_entry.interval);
            }

            auto ax{axes[i]};
            // 使用step绘制阶梯图，post逻辑：第0个点，时刻是0，对应[0, itv]区间的平均CPU利用率
            ax.attr("step")(
                times, m_entry.values, py::arg("where") = "post", py::arg("label") = m_entry.name,
                py::arg("linewidth") = 1.5);
            ax.attr("margins")(py::arg("x") = 0);
            ax.attr("grid")(true, py::arg("linestyle") = "--", py::arg("alpha") = 0.5);
            double ylim{std::max(100., max_value)};
            std::cout << "ylim " << ylim << std::endl;
            ax.attr("set_ylim")(-ylim / 20, ylim + ylim / 20); // 对齐每个子图y坐标范围
            // 绘制每个子图左侧描述raw data的标签
            ax.attr("set_ylabel")(
                "#" + std::to_string(i) + "\nPid: " + std::to_string(r_entry.pid) + "\n" +
                    oops::Elide(r_entry.raw_file_name, 10),
                py::arg("rotation") = 0, py::arg("labelpad") = 30, py::arg("va") = "center");
            ax.attr("legend")(py::arg("loc") = "upper right");
        }

        plt.attr("suptitle")(
            "CPU Utilization Time Series (%)", py::arg("fontsize") = 16, py::arg("fontweight") = "bold");
        plt.attr("xlabel")("Time (s)");
        plt.attr("tight_layout")();
        plt.attr("savefig")("cpu_usage.png", py::arg("dpi") = 100, py::arg("bbox_inches") = "tight");

        if (ARGS.report.show) {
            plt.attr("show")();
        }
        plt.attr("close")("all");
    } catch (const py::error_already_set &e) { std::cerr << "Error: " << e.what() << std::endl; }
}

void Report() {
    // 搜索日志文件
    SearchValidInput();

    // 解析数据
    for (const auto &p : ARGS.report.valid_input) {
        RAW_DATA_TABLE.emplace_back(p.filename().string());
        ParseRawData(p, RAW_DATA_TABLE.back());
    }

    // 对齐时间轴，range裁剪，分辨率缩放
    AlignTimeline();

    // 绘制CPU利用率折线图
    PlotCpu();
}

// 参数解析
void ParseArgs(int argc, char *argv[]) {
    argparse::ArgumentParser program{"monitor", "1.0"};

    argparse::ArgumentParser measure{"measure"};
    measure.add_description("measure runtime performance metrics");
    measure.add_argument("pid").help("target process id to monitor").scan<'i', pid_t>().required();
    measure.add_argument("-i", "--itv")
        .help("sampling interval in secondes (e.g., 0.5 for 500ms)")
        .scan<'g', double>()
        .default_value(1.);
    measure.add_argument("-m", "--metric")
        .help("comma-separated sequence of metrics to monitor (supported metrics: cpu[c], memory[m])")
        .default_value("all");
    program.add_subparser(measure);

    constexpr std::size_t AUTO_RESOLUTION{960};
    argparse::ArgumentParser report{"report"};
    report.add_description("process and visualize monitor raw data");
    report.add_argument("input").help("comma-separated sequence of raw data file").required();
    report.add_argument("-d", "--dir")
        .help("the root directory to search for files matching the 'input' pattern")
        .default_value(".");
    report.add_argument("--resolution")
        .help("number of data points plotted on timeline ('auto', 'max' or an integer)")
        .default_value("auto");
    report.add_argument("-r", "--range")
        .help("subset range for the plot timeline specified as 'start:end', where boundaries must be real or empty")
        .default_value(":");
    report.add_argument("--show").help("display the generated plot in a GUI window").flag();
    program.add_subparser(report);

    try {
        program.parse_args(argc, argv);
    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << std::endl;
        if (program.is_subcommand_used(measure)) {
            std::cerr << measure;
        } else if (program.is_subcommand_used(report)) {
            std::cerr << report;
        } else {
            std::cerr << program;
        }
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
    } else if (program.is_subcommand_used("report")) {
        // program type
        ARGS.program_type = ProgramType::REPORT;

        // input
        std::string input{report.get<std::string>("input")};
        auto input_tokens{oops::Split(input, ',')};
        for (auto token : input_tokens) {
            ARGS.report.input.emplace_back(token);
        }

        // --dir
        std::string dir{report.get<std::string>("--dir")};
        auto dir_tokens{oops::Split(dir, ',')};
        for (auto token : dir_tokens) {
            fs::path dir_path{token};
            if (!fs::is_directory(dir_path)) {
                std::cerr << "Error: '" << token << "' in --dir is not a directory" << std::endl;
                exit(1);
            }
            ARGS.report.dir.emplace_back(token);
        }
        if (ARGS.report.dir.empty()) {
            std::cerr << "Error: no valid directories provided in --dir" << std::endl;
            exit(1);
        }

        // --resolution
        std::string resolution{report.get<std::string>("--resolution")};
        if (resolution == "auto") {
            ARGS.report.resolution = AUTO_RESOLUTION;
        } else if (resolution == "max") {
            ARGS.report.resolution = std::numeric_limits<std::size_t>::max();
        } else {
            auto [ptr, errc]{
                std::from_chars(resolution.data(), resolution.data() + resolution.size(), ARGS.report.resolution)};
            if (errc != std::errc{}) {
                std::cerr << "Error: unexpected resolution '" << resolution << "'" << std::endl;
                exit(1);
            }
        }

        // --range
        std::string range{report.get<std::string>("--range")};
        auto range_tokens{oops::Split(range, ':').To<std::vector>()};
        std::cout << range << " " << range_tokens.size() << std::endl;
        if (range_tokens.size() != 2) {
            std::cerr << "Error: unexpected range format '" << range << "'" << std::endl;
            exit(1);
        }
        double range_start{};
        if (!oops::Strip(range_tokens[0]).empty()) {
            try {
                range_start = std::stod(std::string{range_tokens[0]}); // TODO(resserops): 后续使用fast float库代替
            } catch (...) {
                std::cerr << "Error: unexpected range start '" << range_tokens[0] << "'" << std::endl;
                exit(1);
            }
            if (range_start < 0) {
                std::cerr << "Error: range start '" << range_start << "' must be >= 0" << std::endl;
                exit(1);
            }
            ARGS.report.range.start = std::chrono::duration_cast<std::chrono::system_clock::duration>(
                std::chrono::duration<double>{range_start});
        }
        if (!oops::Strip(range_tokens[1]).empty()) {
            double range_end{};
            try {
                range_end = std::stod(std::string{range_tokens[1]}); // TODO(resserops): 后续使用fast float库代替
            } catch (...) {
                std::cerr << "Error: unexpected range end '" << range_tokens[1] << "'" << std::endl;
                exit(1);
            }
            if (range_end < range_start) {
                std::cerr << "Error: range end '" << range_end << "' must be >= range start '" << range_start << "'"
                          << std::endl;
                exit(1);
            }
            ARGS.report.range.end = std::chrono::duration_cast<std::chrono::system_clock::duration>(
                std::chrono::duration<double>{range_end});
        }

        // --show
        ARGS.report.show = report.is_used("--show");
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
