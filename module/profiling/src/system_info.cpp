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
#include "oops/type_list.h"

namespace oops {
// 获取命令输出
std::string GetCmdOutput(const std::string &cmd) {
    std::unique_ptr<FILE, int (*)(FILE *)> p{popen(cmd.c_str(), "r"), &pclose};
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
bool FromSv(std::string_view sv, T &value) {
    value = FromStr<T>(sv);
    return true;
}

// 新版KeyValueParser
template <typename S, typename F, typename TL>
class KeyValueParser {
    template <typename T>
    using MemberPtr = T S::*; // 辅助元函数生成T S::*成员指针
    using MemberPtrList = oops::meta::TransformT<MemberPtr, TL>;

public:
    static_assert(std::is_enum_v<F>);

    using Struct = S;
    using Field = F;
    using MemberPtrVar = oops::meta::ApplyT<std::variant, MemberPtrList>;

    struct Entry {
        Field field{};
        std::string_view key;
        MemberPtrVar member_ptr_var;
        bool (*parse)(std::string_view, MemberPtrVar, Struct &){ParseField};
        std::string (*format)(MemberPtrVar, const Struct &){FormatField};
    };

    explicit KeyValueParser(std::initializer_list<Entry> entries, std::string delim = ":")
        : field_table_{entries}, delim_{std::move(delim)} {}

    static bool ParseField(std::string_view value, MemberPtrVar member_ptr_var, Struct &obj) {
        auto f = [value, &obj](auto member_ptr) {
            auto &member{obj.*member_ptr};
            return FromSv(value, member);
        };
        return std::visit(f, member_ptr_var);
    }

    static std::string FormatField(MemberPtrVar member_ptr_var, const Struct &obj) {
        auto f = [&obj](auto member_ptr) -> std::string {
            auto &member{obj.*member_ptr};
            return ToStr(member);
        };
        return std::visit(f, member_ptr_var);
    }

    auto Parse(std::istream &is, const EnumBitset<Field> &field_mask) {
        struct {
            Struct object;
            EnumBitset<Field> parsed;
        } res;

        std::string line_buf;
        while (std::getline(is, line_buf)) {
            std::string_view line{line_buf};
            std::size_t pos{line.find(delim_)};
            if (pos == line.npos) {
                continue;
            }

            std::string_view key{Strip(line.substr(0, pos))};
            if (key.empty()) {
                continue;
            }

            auto it{std::find_if(
                field_table_.begin(), field_table_.end(), [&key](const auto &entry) { return entry.key == key; })};
            if (it == field_table_.end()) {
                continue; // 未找到匹配的entry
            }
            if (!field_mask.Test(it->field)) {
                continue; // 不用解析
            }
            if (res.parsed.Test(it->field)) {
                continue; // 已解析过
            }

            std::string_view value{Strip(line.substr(pos + 1))};
            if (value.empty()) {
                continue;
            }

            it->parse(value, it->member_ptr_var, res.object);
            res.parsed |= it->field;
            if (res.parsed == field_mask) {
                break; // 已解析全量
            }
        }
        return res;
    }

    void Format(std::ostream &os, const Struct &object, const EnumBitset<Field> &field_mask = ~EnumBitset<Field>{}) {
        FTable ftable;
        for (const auto &entry : field_table_) {
            if (field_mask.Test(entry.field)) {
                std::string value{entry.format(entry.member_ptr_var, object)};
                if (!value.empty()) {
                    ftable.AppendRow(std::string{entry.key} + delim_, value);
                }
            }
        }
        os << ftable;
    }

private:
    std::vector<Entry> field_table_;
    std::string delim_;
};

namespace proc {
namespace status {
KeyValueParser<Info, Field, std::tuple<std::size_t>> parser{
    {Field::VM_PEAK, "VmPeak", &Info::vm_peak},       {Field::VM_SIZE, "VmSize", &Info::vm_size},
    {Field::VM_LCK, "VmLck", &Info::vm_lck},          {Field::VM_PIN, "VmPin", &Info::vm_pin},
    {Field::VM_HWM, "VmHWM", &Info::vm_hwm},          {Field::VM_RSS, "VmRSS", &Info::vm_rss},
    {Field::RSS_ANON, "RssAnon", &Info::rss_anon},    {Field::RSS_FILE, "RssFile", &Info::rss_file},
    {Field::RSS_SHMEM, "RssShmem", &Info::rss_shmem}, {Field::VM_DATA, "VmData", &Info::vm_data},
    {Field::VM_STK, "VmStk", &Info::vm_stk},          {Field::VM_EXE, "VmExe", &Info::vm_exe},
    {Field::VM_LIB, "VmLib", &Info::vm_lib},          {Field::VM_PTE, "VmPTE", &Info::vm_pte},
    {Field::VM_SWAP, "VmSwap", &Info::vm_swap}};

Info Get(std::ifstream &ifs, const FieldMask &field_mask) {
    auto [info, parsed]{parser.Parse(ifs, field_mask)};
    info.parsed = std::move(parsed);
    return info;
}

Info Get() { return Get(~FieldMask{}); }
Info Get(int pid) { return Get(pid, ~FieldMask{}); }

Info Get(const FieldMask &field_mask) {
    std::ifstream ifs("/proc/self/status");
    return Get(ifs, field_mask);
}

Info Get(int pid, const FieldMask &field_mask) {
    std::ifstream ifs(std::string{"/proc/"} + std::to_string(pid) + "/status");
    return Get(ifs, field_mask);
}

std::ostream &operator<<(std::ostream &out, const Info &info) {
    parser.Format(out, info, info.parsed);
    return out;
}
} // namespace status
namespace numa_maps {
// TODO(resserops): 补充实现
} // namespace numa_maps
} // namespace proc

namespace lscpu {
KeyValueParser<Info, Field, std::tuple<std::size_t, double, std::string>> parser{
    {Field::ARCHITECTURE, "Architecture", &Info::architecture},
    {Field::CPUS, "CPU(s)", &Info::cpus},
    {Field::THREADS_PER_CORE, "Thread(s) per core", &Info::threads_per_core},
    {Field::CORES_PER_SOCKET, "Core(s) per socket", &Info::cores_per_socket},
    {Field::SOCKETS, "Socket(s)", &Info::sockets},
    {Field::NUMA_NODES, "NUMA node(s)", &Info::numa_nodes},
    {Field::MODEL_NAME, "Model name", &Info::model_name},
    {Field::CPU_MHZ, "CPU MHz", &Info::cpu_mhz}};

Info Get() { return Get(~FieldMask{}); }
Info Get(const FieldMask &field_mask) {
    std::istringstream iss{GetCmdOutput("lscpu")};
    auto [info, parsed]{parser.Parse(iss, field_mask)};
    info.parsed = std::move(parsed);
    return info;
}

std::ostream &operator<<(std::ostream &out, const Info &info) {
    parser.Format(out, info, info.parsed);
    return out;
}
} // namespace lscpu
} // namespace oops
