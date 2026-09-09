#include "oops/proc/task/vma.h"

#include <istream>
#include <ostream>
#include <string>
#include <tuple>

#include <sys/sysmacros.h>

#include "fmt/format.h"
#include "scn/scan.h"

#include "oops/str.h"

namespace oops {
namespace proc {
namespace task {
std::uint32_t Vma::MajorDev() const { return major(dev); }
std::uint32_t Vma::MinorDev() const { return minor(dev); }

ParseVmaResult ParseVma(std::istream &is) {
    ParseVmaResult res;

    std::streampos checkpoint{is.tellg()};
    std::string buf;
    if (!std::getline(is, buf)) {
        res.failed = true;
        return res;
    }

    auto scan_res{scn::scan<uintptr_t, uintptr_t, char, char, char, char, off_t, uint32_t, uint32_t, ino_t>(
        buf, "{:x}-{:x} {}{}{}{} {:x} {:x}:{:x} {}")};
    if (!scan_res) {
        checkpoint = is.tellg();
        res.failed = true;
        return res;
    }
    checkpoint = is.tellg();

    const auto &values{scan_res->values()};
    res.vma.address.start = std::get<0>(values);
    res.vma.address.end = std::get<1>(values);

    res.vma.perms.r = (std::get<2>(values) == 'r');
    res.vma.perms.w = (std::get<3>(values) == 'w');
    res.vma.perms.x = (std::get<4>(values) == 'x');
    res.vma.perms.s = (std::get<5>(values) == 's');

    res.vma.offset = std::get<6>(values);
    res.vma.dev = makedev(std::get<7>(values), std::get<8>(values));
    res.vma.inode = std::get<9>(values);

    // scan剩余字符串即pathname
    auto range{scan_res->range()};
    res.vma.pathname = Strip({&*range.begin(), range.size()});
    return res;
}

void FormatVma(std::ostream &os, const Vma &vma) {
    char r{vma.perms.r ? 'r' : '-'};
    char w{vma.perms.w ? 'w' : '-'};
    char x{vma.perms.x ? 'x' : '-'};
    char p{vma.perms.s ? 's' : 'p'};

    std::string fmt_res{fmt::format(
        "{:x}-{:x} {}{}{}{} {:08x} {:02x}:{:02x} {}", vma.address.start, vma.address.end, r, w, x, p, vma.offset,
        vma.MajorDev(), vma.MinorDev(), vma.inode)};

    if (vma.pathname.empty()) {
        os << fmt_res;
    } else {
        os << fmt::format("{:<70}{}", fmt_res, vma.pathname);
    }
    os << "\n";
}
} // namespace task
} // namespace proc
} // namespace oops
