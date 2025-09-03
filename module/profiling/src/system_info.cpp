#include "oops/system_info.h"

#include <cstdio>
#include <memory>
#include <stdexcept>
#include <sstream>
#include <functional>
#include <charconv>
#include <cassert>
#include <string_view>
#include <functional>
#include <fstream>
#include <iostream>

#include "oops/str.h"
#include "oops/format.h"

#define GET(member) [](const Info &info) -> std::string { return info.member ? ToStr(*info.member) : ""; }
#define SET(member) [](std::string_view value, Info &info){ return FromSv(value, info.member); }
#define ACCESS(member) GET(member), SET(member)

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

template <typename Info, typename Field>
struct FieldMeta {
    std::string_view key;
    std::function<std::string(const Info &)> get;
    std::function<bool(std::string_view, Info&)> set;
};

template <typename Info, typename Field>
Info GetPairedInfo(std::istream &is, const std::vector<FieldMeta<Info, Field>> &table, const EnumBitset<Field> &field_mask) {
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
        for (; i < table.size(); ++i) {
            if (field_mask[i] && table[i].key == key) {
                break;
            }
        }
        if (i >= table.size()) {
            continue;
        }

        std::string_view value{str::Strip(line.substr(pos + 1))};
        if (value.empty()) {
            continue;
        }
        
        table[i].set(value, info);
        checked |= static_cast<Field>(i);
        if (checked == field_mask) {
            break;
        }
    }
    return info;
}

template <typename Info, typename Field>
void OutputPairedInfo(std::ostream &os, const Info &info, const std::vector<FieldMeta<Info, Field>> &table, const EnumBitset<Field> &) {  // TODO(resserops): 传入bitmap判断打印哪些内容
    FTable ftable;
    for (const auto &meta : table) {
        std::string value{meta.get(info)};
        if (!value.empty()) {
            ftable.AppendRow(std::string{meta.key} + ":", value);
        }
    }
    os << ftable;
}

namespace status {
static const std::vector<FieldMeta<Info, Field>> TABLE{
    {"VmPeak", ACCESS(vm_peak)},
    {"VmSize", ACCESS(vm_size)},
    {"VmLck", ACCESS(vm_lck)},
    {"VmPin", ACCESS(vm_pin)},
    {"VmHWM", ACCESS(vm_hwm)},
    {"VmRSS", ACCESS(vm_rss)},
    {"RssAnon", ACCESS(rss_anon)},
    {"RssFile", ACCESS(rss_file)},
    {"RssShmem", ACCESS(rss_shmem)},
    {"VmData", ACCESS(vm_data)},
    {"VmStk", ACCESS(vm_stk)},
    {"VmExe", ACCESS(vm_exe)},
    {"VmLib", ACCESS(vm_lib)},
    {"VmPTE", ACCESS(vm_pte)},
    {"VmSwap", ACCESS(vm_swap)}
};

Info Get() {
    return Get(~FieldMask{});
}

Info Get(const FieldMask &field_mask) {
    std::ifstream ifs("/proc/self/status");
    return GetPairedInfo(ifs, TABLE, field_mask);
}

std::ostream& operator<<(std::ostream &out, const Info &info) {
    OutputPairedInfo(out, info, TABLE, {});
    return out;
}
}

namespace lscpu {
static const std::vector<FieldMeta<Info, Field>> TABLE{
    {"Architecture", ACCESS(architecture)},
    {"CPU(s)", ACCESS(cpus)},
    {"Thread(s) per core", ACCESS(threads_per_core)},
    {"Core(s) per socket", ACCESS(cores_per_socket)},
    {"Socket(s)", ACCESS(sockets)},
    {"NUMA node(s)", ACCESS(numa_nodes)},
    {"Model name", ACCESS(model_name)},
    {"CPU MHz", ACCESS(cpu_mhz)}
};

Info Get() {
    return Get(~FieldMask{});
}

Info Get(const FieldMask &field_mask) {
    std::istringstream iss{GetCmdOutput("lscpu")};
    return GetPairedInfo(iss, TABLE, field_mask);
}

std::ostream& operator<<(std::ostream &out, const Info &info) {
    OutputPairedInfo(out, info, TABLE, {});
    return out;
}
}
}
