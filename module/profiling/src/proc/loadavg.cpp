#include "oops/proc/loadavg.h"

#include <fstream>
#include <istream>
#include <ostream>

namespace oops {
namespace proc {
namespace loadavg {
Info Get() {
    std::ifstream ifs{"/proc/loadavg"};
    return Get(ifs);
}

Info Get(std::istream &is) {
    Info info;
    char slash{};

    is >> info.load1 >> info.load5 >> info.load15 >> info.sched_entities.running >> slash >>
        info.sched_entities.total >> info.last_pid;

    info.parsed = (!is.fail() && slash == '/');
    return info;
}

std::ostream &operator<<(std::ostream &os, const Info &info) {
    if (!info.parsed) {
        return os;
    }
    // TODO(resserops): flags和precision的保存、复位，后续抽象成guard
    const auto flags{os.flags()};
    const auto precision{os.precision()};
    os.setf(std::ios::fixed, std::ios::floatfield);
    os.precision(2);

    os << info.load1 << ' ' << info.load5 << ' ' << info.load15 << ' ' << info.sched_entities.running << '/'
       << info.sched_entities.total << ' ' << info.last_pid << '\n';

    os.flags(flags);
    os.precision(precision);
    return os;
}
} // namespace loadavg
} // namespace proc
} // namespace oops
