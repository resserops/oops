#include "oops/lscpu.h"

#include <istream>
#include <memory>
#include <ostream>
#include <sstream>
#include <string>

#include <cstdio>

#include "oops/key_value_parser.h"

namespace oops {
namespace lscpu {
namespace {
// 获取命令行输出
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

KeyValueParser<Info, Field, meta::TypeList<std::size_t, double, std::string>> kvparser{
    {{Field::ARCHITECTURE, "Architecture", &Info::architecture},
     {Field::CPUS, "CPU(s)", &Info::cpus},
     {Field::THREADS_PER_CORE, "Thread(s) per core", &Info::threads_per_core},
     {Field::CORES_PER_SOCKET, "Core(s) per socket", &Info::cores_per_socket},
     {Field::SOCKETS, "Socket(s)", &Info::sockets},
     {Field::NUMA_NODES, "NUMA node(s)", &Info::numa_nodes},
     {Field::MODEL_NAME, "Model name", &Info::model_name},
     {Field::CPU_MHZ, "CPU MHz", &Info::cpu_mhz}}};
} // namespace

Info Get() { return Get(~FieldMask{}); }
Info Get(const FieldMask &field_mask) {
    Info info;
    std::istringstream iss{GetCmdOutput("lscpu")};
    info.parsed |= kvparser.Parse(iss, info, field_mask);
    return info;
}

std::ostream &operator<<(std::ostream &os, const Info &info) {
    kvparser.Format(os, info, info.parsed);
    return os;
}
} // namespace lscpu
} // namespace oops
