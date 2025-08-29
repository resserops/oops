#include "oops/system_info.h"

#include <memory>
#include <stdexcept>
#include <sstream>
#include <functional>
#include <charconv>

#include "oops/str.h"

namespace oops {
namespace lscpu {
struct PopenDeleter {
    void operator()(FILE* ptr) const {
        if (ptr) {
            pclose(ptr);
        }
    }
};

std::string GetCmdOutput(const std::string &cmd) {
    std::unique_ptr<FILE, PopenDeleter> pipe(popen(cmd.c_str(), "r"));
    if (!pipe) {
        throw std::runtime_error("popen() failed!");
    }

    char buffer[256];
    std::string output;
    while (fgets(buffer, sizeof buffer, pipe.get()) != nullptr) {
        output += buffer;
    }
    return output;
}

struct Item {
    std::string_view key{};
    std::function<void(std::string_view, Info&)> f{};
};

template <typename T>
void FromSv(std::string_view sv, T &value) {
    const char *begin{sv.data()};
    std::from_chars(begin, begin + sv.size(), value);
}

void FromSv(std::string_view sv, std::string &value) {
    value = sv;
}

void FromSv(std::string_view sv, double &value) {
    value = std::stod(std::string(sv));
}

void FromSv(std::string_view sv, float &value) {
    value = std::stof(std::string(sv));
}

#define CVT_FUN(member) [](std::string_view value, Info &info) { FromSv(value, info.member); }

Info Get() {
    std::istringstream iss{GetCmdOutput("lscpu")};
    std::string buf;
    Info info;
    static const std::vector<Item> table{
        {"Architecture", CVT_FUN(architecture)},
        {"CPU(s)", CVT_FUN(cpus)},
        {"Thread(s) per core", CVT_FUN(threads_per_core)},
        {"Core(s) per socket", CVT_FUN(cores_per_socket)},
        {"Socket(s)", CVT_FUN(sockets)},
        {"NUMA node(s)", CVT_FUN(numa_nodes)},
        {"Model name", CVT_FUN(model_name)},
        {"CPU MHz", CVT_FUN(cpu_mhz)}
    };
    while (std::getline(iss, buf)) {
        std::string_view line{buf};
        size_t pos{line.find(':')};
        if (pos == line.npos) {
            continue;
        }
        std::string_view key{line.substr(0, pos)};
        std::string_view value{line.substr(pos + 1)};
        constexpr const char SPACE[]{" \t\n\r\v\f"};
        size_t pos0{value.find_first_not_of(SPACE)};
        value = value.substr(pos0, value.find_last_not_of(SPACE) - pos0 + 1);
        for (auto &item : table) {
            if (key == item.key) {
                item.f(value, info);
                break;
            }
        }
    }
    return info;
}
}
}
