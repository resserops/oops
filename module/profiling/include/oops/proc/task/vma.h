#pragma once
#include <cstdint>
#include <iosfwd>
#include <string>

#include <sys/types.h>

namespace oops {
namespace proc {
namespace task {
struct Vma {
    std::size_t Size() const { return static_cast<std::size_t>(address.end - address.start); }
    std::uint32_t MajorDev() const;
    std::uint32_t MinorDev() const;

    struct Address {
        std::uintptr_t start{};
        std::uintptr_t end{};
    } address{};

    struct Perms {
        bool r : 1;
        bool w : 1;
        bool x : 1;
        bool s : 1; // s(shared) or p(private)
    } perms{};

    off_t offset{};
    dev_t dev{};
    ino_t inode{};
    std::string pathname{};
};

struct ParseVmaResult {
    explicit operator bool() const noexcept { return !failed; }
    Vma vma;
    bool failed{};
};
ParseVmaResult ParseVma(std::istream &is);
void FormatVma(std::ostream &os, const Vma &vma);
} // namespace task
} // namespace proc
} // namespace oops
