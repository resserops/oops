#include "oops/system_info.h"

#include <array>
#include <cassert>
#include <cctype>
#include <charconv>
#include <cstdio>
#include <fstream>
#include <functional>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string_view>
#include <variant>

#include "oops/format.h"
#include "oops/str.h"

#define ENTRY(key, field, member, parse, format) \
    oops::FieldEntry<Field, Info, MemberPtrVar>({Field::field, #field, key, #member, &Info::member, parse, format})

namespace oops {
// 获取命令输出
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

// 单字段的提取、解析
template <typename T>
bool FromSv(std::string_view sv, std::optional<T> &value) {
    value = str::FromStr<T>(sv);
    return true;
}

template <typename Info, typename MemberPtrVar>
bool Parse(std::string_view value, MemberPtrVar member_ptr_var, Info &info) {
    auto f = [value, &info](auto member_ptr) {
        auto &member{info.*member_ptr};
        return FromSv(value, member);
    };
    return std::visit(f, member_ptr_var);
}

template <typename Info, typename MemberPtrVar>
std::string Format(MemberPtrVar member_ptr_var, const Info &info) {
    auto f = [&info](auto member_ptr) -> std::string {
        auto &member{info.*member_ptr};
        if (!member) {
            return {};
        }
        return ToStr(*member);
    };
    return std::visit(f, member_ptr_var);
}

// 用于解析PairedInfo的字段表
template <typename Field, typename Info, typename MemberPtrVar>
struct FieldEntry {
    Field field;
    std::string_view field_str;
    std::string_view key;
    std::string_view member_str;
    MemberPtrVar member_ptr_var;
    bool (*parse)(std::string_view, MemberPtrVar, Info &);
    std::string (*format)(MemberPtrVar, const Info &);
};

template <typename Field, typename Info, typename MemberPtrVar>
using FieldTable = std::array<FieldEntry<Field, Info, MemberPtrVar>, ToUnderlying(Field::COUNT)>;

// 判断字段表是否合法的编译期检查函数
constexpr bool CaseInsensitiveEqual(char lhs, char rhs) noexcept { return str::ToLower(lhs) == str::ToLower(rhs); }

template <typename Field, typename Info, typename MemberPtrVar>
constexpr bool CheckFieldTableMapping(const FieldTable<Field, Info, MemberPtrVar> &field_table) {
    if (field_table.size() != ToUnderlying(Field::COUNT)) {
        return false;
    }
    for (std::size_t i{0}; i < field_table.size(); ++i) {
        if (i != static_cast<std::size_t>(ToUnderlying(field_table[i].field))) {
            return false;
        }
        if (!str::Equal(field_table[i].field_str, field_table[i].member_str, CaseInsensitiveEqual)) {
            return false;
        }
        if (!str::Equal(field_table[i].key, field_table[i].field_str, CaseInsensitiveEqual, str::IsAlnum)) {
            return false;
        }
    }
    return true;
}

// 针对PariedInfo解析和打印的通用函数
template <typename Field, typename Info, typename MemberPtrVar>
Info GetPairedInfo(
    std::istream &is, const FieldTable<Field, Info, MemberPtrVar> &field_table, const EnumBitset<Field> &field_mask) {
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

        field_table[i].parse(value, field_table[i].member_ptr_var, info);
        checked |= static_cast<Field>(i);
        if (checked == field_mask) {
            break;
        }
    }
    return info;
}

// TODO(resserops): 传入bitmap判断打印哪些内容
template <typename Field, typename Info, typename MemberPtrVar>
void OutputPairedInfo(
    std::ostream &os, const Info &info, const FieldTable<Field, Info, MemberPtrVar> &field_table,
    const EnumBitset<Field> &) {
    FTable ftable;
    for (const auto &meta : field_table) {
        std::string value{meta.format(meta.member_ptr_var, info)};
        if (!value.empty()) {
            ftable.AppendRow(std::string{meta.key} + ":", value);
        }
    }
    os << ftable;
}

namespace proc {
namespace status {
using MemberPtrVar = std::variant<std::optional<size_t> Info::*>; // 定义所有类型的成员变量指针
constexpr auto PARSE{oops::Parse<Info, MemberPtrVar>};
constexpr auto FORMAT{oops::Format<Info, MemberPtrVar>};

using FieldTable = oops::FieldTable<Field, Info, MemberPtrVar>;
static constexpr FieldTable FIELD_TABLE{
    ENTRY("VmPeak", VM_PEAK, vm_peak, PARSE, FORMAT),       ENTRY("VmSize", VM_SIZE, vm_size, PARSE, FORMAT),
    ENTRY("VmLck", VM_LCK, vm_lck, PARSE, FORMAT),          ENTRY("VmPin", VM_PIN, vm_pin, PARSE, FORMAT),
    ENTRY("VmHWM", VM_HWM, vm_hwm, PARSE, FORMAT),          ENTRY("VmRSS", VM_RSS, vm_rss, PARSE, FORMAT),
    ENTRY("RssAnon", RSS_ANON, rss_anon, PARSE, FORMAT),    ENTRY("RssFile", RSS_FILE, rss_file, PARSE, FORMAT),
    ENTRY("RssShmem", RSS_SHMEM, rss_shmem, PARSE, FORMAT), ENTRY("VmData", VM_DATA, vm_data, PARSE, FORMAT),
    ENTRY("VmStk", VM_STK, vm_stk, PARSE, FORMAT),          ENTRY("VmExe", VM_EXE, vm_exe, PARSE, FORMAT),
    ENTRY("VmLib", VM_LIB, vm_lib, PARSE, FORMAT),          ENTRY("VmPTE", VM_PTE, vm_pte, PARSE, FORMAT),
    ENTRY("VmSwap", VM_SWAP, vm_swap, PARSE, FORMAT)};

static_assert(CheckFieldTableMapping(FIELD_TABLE));

Info Get() { return Get(~FieldMask{}); }
Info Get(int pid) { return Get(pid, ~FieldMask{}); }

Info Get(const FieldMask &field_mask) {
    std::ifstream ifs("/proc/self/status");
    return GetPairedInfo(ifs, FIELD_TABLE, field_mask);
}

Info Get(int pid, const FieldMask &field_mask) {
    std::ifstream ifs(std::string{"/proc/"} + std::to_string(pid) + "/status");
    return GetPairedInfo(ifs, FIELD_TABLE, field_mask);
}

std::ostream &operator<<(std::ostream &out, const Info &info) {
    OutputPairedInfo(out, info, FIELD_TABLE, FieldMask{});
    return out;
}
} // namespace status
namespace numa_maps {
using MemberPtrVar = std::variant<uintptr_t MemoryRange::*, std::optional<size_t> MemoryRange::*, >; // 定义所有类型的成员变量指针
constexpr auto PARSE{oops::Parse<Info, MemberPtrVar>};
constexpr auto FORMAT{oops::Format<Info, MemberPtrVar>};

Info Get() {
    std::ifstream ifs("/proc/self/numa_maps");
    std::string line_buf;
    while (std::getline(ifs, line_buf)) {
        std::string_view line{line_buf};
        std::vector<std::string> sd;
    }
}
} // namespace numa_maps
} // namespace proc

namespace lscpu {
using MemberPtrVar = std::variant<
    std::optional<size_t> Info::*, std::optional<std::string> Info::*,
    std::optional<double> Info::*>; // 定义所有类型的成员变量指针
constexpr auto PARSE{oops::Parse<Info, MemberPtrVar>};
constexpr auto FORMAT{oops::Format<Info, MemberPtrVar>};

using FieldTable = oops::FieldTable<Field, Info, MemberPtrVar>;
static constexpr FieldTable FIELD_TABLE{
    ENTRY("Architecture", ARCHITECTURE, architecture, PARSE, FORMAT),
    ENTRY("CPU(s)", CPUS, cpus, PARSE, FORMAT),
    ENTRY("Thread(s) per core", THREADS_PER_CORE, threads_per_core, PARSE, FORMAT),
    ENTRY("Core(s) per socket", CORES_PER_SOCKET, cores_per_socket, PARSE, FORMAT),
    ENTRY("Socket(s)", SOCKETS, sockets, PARSE, FORMAT),
    ENTRY("NUMA node(s)", NUMA_NODES, numa_nodes, PARSE, FORMAT),
    ENTRY("Model name", MODEL_NAME, model_name, PARSE, FORMAT),
    ENTRY("CPU MHz", CPU_MHZ, cpu_mhz, PARSE, FORMAT)};

static_assert(CheckFieldTableMapping(FIELD_TABLE));

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
