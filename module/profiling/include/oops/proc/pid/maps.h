#pragma once

#include <cstdint>
#include <iosfwd>
#include <string>
#include <vector>

#include <sys/types.h> // 提供内核数据结构

namespace oops {
namespace proc {
namespace pid {
namespace maps {
// vma数据结构，maps, smaps, smaps_rollup共用
struct Vma {
    std::size_t Size() const { return static_cast<std::size_t>(address.start - address.end); }
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

struct Info {
    std::vector<Vma> vma_table;
};

// ParseVma/FormatVma供smaps, smaps_rollup复用
struct ParseVmaResult {
    explicit operator bool() const noexcept { return !failed; }
    Vma vma;
    bool failed{};
};

[[nodiscard]] Info Get();
[[nodiscard]] Info Get(pid_t pid);
[[nodiscard]] ParseVmaResult ParseVma(std::istream &is);
void FormatVma(std::ostream &os, const Vma &vma);
std::ostream &operator<<(std::ostream &os, const Info &info);
} // namespace maps
} // namespace pid
} // namespace proc
} // namespace oops
