#pragma once
#include <cstdint>
#include <string>
#include <type_traits>
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

    FTable &SetDelim(const ::std::string &delim);
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

    void Output(::std::ostream &out) const;

private:
    const Prop &GetProp(std::size_t j) const;

    ::std::string delim_{" "};
    ::std::vector<::std::vector<::std::string>> table_{{}};
    ::std::vector<Prop> col_prop_vec_;
    Prop table_prop_;
};

// 为所有定义Output函数的类重载<<运算符
template <typename T, typename = void>
struct HasOutput : ::std::false_type {};
template <typename T>
struct HasOutput<T, ::std::void_t<decltype(::std::declval<T>().Output(::std::declval<::std::ostream &>()))>>
    : ::std::true_type {};
template <typename T>
typename ::std::enable_if<HasOutput<T>::value, ::std::ostream &>::type operator<<(::std::ostream &os, const T &obj) {
    obj.Output(os);
    return os;
}
} // namespace oops
