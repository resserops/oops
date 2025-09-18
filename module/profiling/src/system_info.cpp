#include "oops/system_info.h"

#include <cassert>
#include <charconv>
#include <cstdio>
#include <fstream>
#include <functional>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string_view>

#include "oops/format.h"
#include "oops/str.h"

#define REG(parse, format, member) parse(member), format(member)

#define PARSE(member) [](std::string_view value, Info &info) { return FromSv(value, info.member); }
#define FORMAT(member) [](const Info &info) -> std::string { return info.member ? ToStr(*info.member) : ""; }

namespace oops {
std::string GetCmdOutput(const std::string &cmd) {
    std::unique_ptr<FILE, decltype(&pclose)> p{popen(cmd.c_str(), "r"), &pclose};
    std::string output;
    if (!p) {
        return output;
    }

    char buffer[4096];
    while (fgets(buffer, sizeof(buffer), p.get()) != nullptr) {
        output += buffer;
    }
    return output;
}

template <typename T>
bool FromSv(std::string_view sv, std::optional<T> &value) {
    value = str::FromStr<T>(sv);
    return true;
}

template <typename Info>
struct FieldEntry {
    std::string_view key;
    std::function<bool(std::string_view, Info &)> parse;
    std::function<std::string(const Info &)> format;
};

template <typename Info, typename Field>
Info GetPairedInfo(
    std::istream &is, const std::vector<FieldEntry<Info>> &field_table, const EnumBitset<Field> &field_mask) {
    Info info;
    std::string line_buf;

    EnumBitset<Field> checked;
    while (std::getline(is, line_buf)) {
        std::string_view line{line_buf};
        std::size_t pos{line.find(':')};
        if (pos == line.npos) {
            continue;
        }

        std::string_view key{str::Strip(line.substr(0, pos))};
        if (key.empty()) {
            continue;
        }

        std::size_t i{0};
        for (; i < field_table.size(); ++i) {
            if (field_mask[i] && field_table[i].key == key) {
                break;
            }
        }
        if (i >= field_table.size()) {
            continue;
        }

        std::string_view value{str::Strip(line.substr(pos + 1))};
        if (value.empty()) {
            continue;
        }

        field_table[i].parse(value, info);
        checked |= static_cast<Field>(i);
        if (checked == field_mask) {
            break;
        }
    }
    return info;
}

template <typename Info, typename Field>
void OutputPairedInfo(
    std::ostream &os, const Info &info, const std::vector<FieldEntry<Info>> &field_table,
    const EnumBitset<Field> &) { // TODO(resserops): 传入bitmap判断打印哪些内容
    FTable ftable;
    for (const auto &meta : field_table) {
        std::string value{meta.format(info)};
        if (!value.empty()) {
            ftable.AppendRow(std::string{meta.key} + ":", value);
        }
    }
    os << ftable;
}

namespace status {
static const std::vector<FieldEntry<Info>> FIELD_TABLE{
    {"VmPeak", REG(PARSE, FORMAT, vm_peak)},     {"VmSize", REG(PARSE, FORMAT, vm_size)},
    {"VmLck", REG(PARSE, FORMAT, vm_lck)},       {"VmPin", REG(PARSE, FORMAT, vm_pin)},
    {"VmHWM", REG(PARSE, FORMAT, vm_hwm)},       {"VmRSS", REG(PARSE, FORMAT, vm_rss)},
    {"RssAnon", REG(PARSE, FORMAT, rss_anon)},   {"RssFile", REG(PARSE, FORMAT, rss_file)},
    {"RssShmem", REG(PARSE, FORMAT, rss_shmem)}, {"VmData", REG(PARSE, FORMAT, vm_data)},
    {"VmStk", REG(PARSE, FORMAT, vm_stk)},       {"VmExe", REG(PARSE, FORMAT, vm_exe)},
    {"VmLib", REG(PARSE, FORMAT, vm_lib)},       {"VmPTE", REG(PARSE, FORMAT, vm_pte)},
    {"VmSwap", REG(PARSE, FORMAT, vm_swap)}};

Info Get() { return Get(~FieldMask{}); }
Info Get(const FieldMask &field_mask) {
    std::ifstream ifs("/proc/self/status");
    return GetPairedInfo(ifs, FIELD_TABLE, field_mask);
}

std::ostream &operator<<(std::ostream &out, const Info &info) {
    OutputPairedInfo(out, info, FIELD_TABLE, FieldMask{});
    return out;
}
} // namespace status

namespace lscpu {
static const std::vector<FieldEntry<Info>> FIELD_TABLE{
    {"Architecture", REG(PARSE, FORMAT, architecture)},
    {"CPU(s)", REG(PARSE, FORMAT, cpus)},
    {"Thread(s) per core", REG(PARSE, FORMAT, threads_per_core)},
    {"Core(s) per socket", REG(PARSE, FORMAT, cores_per_socket)},
    {"Socket(s)", REG(PARSE, FORMAT, sockets)},
    {"NUMA node(s)", REG(PARSE, FORMAT, numa_nodes)},
    {"Model name", REG(PARSE, FORMAT, model_name)},
    {"CPU MHz", REG(PARSE, FORMAT, cpu_mhz)}};

Info Get() { return Get(~FieldMask{}); }
Info Get(const FieldMask &field_mask) {
    std::istringstream iss{GetCmdOutput("lscpu")};
    return GetPairedInfo(iss, FIELD_TABLE, field_mask);
}

std::ostream &operator<<(std::ostream &out, const Info &info) {
    OutputPairedInfo(out, info, FIELD_TABLE, FieldMask{});
    return out;
}
} // namespace lscpu
} // namespace oops
