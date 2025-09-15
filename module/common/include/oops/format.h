#pragma once

#include <iomanip>
#include <sstream>
#include <string>
#include <type_traits>
#include <vector>

#include "oops/str.h"

namespace oops {
template <typename F>
class FFloatPoint {
    static_assert(::std::is_floating_point_v<F>);
    enum Format : uint8_t { FIXED, SCI };

public:
    explicit FFloatPoint(F f) : f_{f} {}
    FFloatPoint &Sci();
    FFloatPoint &Fixed();
    FFloatPoint &SetPrecision(uint8_t precision);
    void Output(::std::ostream &out) const;

private:
    Format format_{FIXED};
    uint8_t precision_{2};
    F f_;
};

using FFloat = FFloatPoint<float>;
using FDouble = FFloatPoint<double>;

class FTable {
public:
    enum Align : uint8_t { LEFT, CENTER, RIGHT };

    struct Prop {
        Align align{LEFT};
        size_t left_margin{0};
        size_t right_margin{0};
        size_t min_width{0};
    };

    FTable &SetDelim(const ::std::string &delim);
    FTable &SetProp(const Prop &prop);
    FTable &SetProp(size_t j, const Prop &prop);

    template <typename... Args>
    FTable &AppendRow(const Args &...args) {
        table_.back() = {ToStr(args)...};
        table_.emplace_back();
        return *this;
    }
    template <typename T>
    FTable &Append(const T &t, bool end = false) {
        table_.back().push_back({ToStr(t)});
        if (end) {
            table_.emplace_back();
        }
        return *this;
    }

    void Output(::std::ostream &out) const;

private:
    const Prop &GetProp(size_t j) const;

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
