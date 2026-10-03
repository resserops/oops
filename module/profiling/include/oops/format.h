#pragma once
#include <cstdint>
#include <string>
#include <vector>

#include "fmt/format.h"

namespace oops {
class FTable {
public:
    enum Align : std::uint8_t { LEFT, CENTER, RIGHT };

    struct Prop {
        Align align{LEFT};
        std::size_t left_margin{0};
        std::size_t right_margin{0};
        std::size_t min_width{0};
    };

    FTable &SetDelim(const std::string &delim);
    FTable &SetProp(const Prop &prop);
    FTable &SetProp(std::size_t j, const Prop &prop);

    template <typename... Args>
    FTable &AppendRow(const Args &...args) {
        table_.back() = {fmt::format("{}", args)...};
        table_.emplace_back();
        return *this;
    }
    template <typename T>
    FTable &Append(const T &t, bool end = false) {
        table_.back().push_back({fmt::format("{}", t)});
        if (end) {
            table_.emplace_back();
        }
        return *this;
    }

    void Output(std::ostream &out) const;

private:
    const Prop &GetProp(std::size_t j) const;

    std::string delim_{" "};
    std::vector<std::vector<std::string>> table_{{}};
    std::vector<Prop> col_prop_vec_;
    Prop table_prop_;
};

std::ostream &operator<<(std::ostream &os, const FTable &table);
} // namespace oops
