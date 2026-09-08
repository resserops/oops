#pragma once

#include <iostream>
#include <istream>
#include <ostream>
#include <string>
#include <string_view>
#include <type_traits>
#include <variant>
#include <vector>

#include "fmt/format.h"
#include "scn/scan.h"

#include "oops/enum_bitset.h"
#include "oops/str.h"
#include "oops/type_list.h"
#include "oops/unit.h"

namespace oops {
template <typename T>
bool ParseField(std::string_view s, T &t, std::string_view ctx) {
    if (ctx.empty()) {
        ctx = "{}";
    }
    auto scan_res{scn::scan<T>(s, ctx)};
    if (scan_res) {
        t = scan_res->value();
    }
    return bool{scan_res};
}

template <typename T>
std::string FormatField(const T &t, std::string_view ctx) {
    if (ctx.empty()) {
        ctx = "{}";
    }
    return fmt::format(ctx, t);
}

// std::string特化
template <>
inline bool ParseField(std::string_view s, std::string &t, std::string_view) {
    t = s;
    return true;
}

// std::vector<std::string>特化
template <>
inline bool ParseField(std::string_view s, std::vector<std::string> &v, std::string_view) {
    v.clear();
    for (auto token : Split(s)) {
        v.emplace_back(token);
    }
    return true;
}

template <>
inline std::string FormatField(const std::vector<std::string> &v, std::string_view) {
    std::string s;
    s.reserve(64); // one cache line
    for (const auto &item : v) {
        s += item;
        s += ' ';
    }
    if (!s.empty()) {
        s.pop_back();
    }
    return s;
}

// bool特化：支持在ctx中定义代表真假的字符串，例如"true/false"或"yes/no"
template <>
inline bool ParseField(std::string_view s, bool &b, std::string_view ctx) {
    std::string_view t, f;
    if (ctx.empty()) {
        t = "1";
        f = "0";
    } else {
        auto pos{ctx.find('/')};
        if (pos == std::string_view::npos) {
            return false;
        }
        t = Strip(ctx.substr(0, pos));
        f = Strip(ctx.substr(pos + 1));
    }

    if (s == t) {
        b = true;
        return true;
    }
    if (s == f) {
        b = false;
        return true;
    }
    return false;
}

template <>
inline std::string FormatField(const bool &b, std::string_view ctx) {
    std::string_view t, f;
    auto pos{ctx.find('/')};
    if (pos == std::string_view::npos) {
        t = "1";
        f = "0";
    } else {
        t = Strip(ctx.substr(0, pos));
        f = Strip(ctx.substr(pos + 1));
    }
    return std::string{b ? t : f};
}

// Storage特化
template <typename R, typename P>
bool ParseField(std::string_view s, Storage<R, P> &storage, std::string_view ctx) {
    R r{};
    auto res{ParseField<R>(s, r, ctx)};
    storage = Storage<R, P>{r};
    return res;
}

template <typename R, typename P>
std::string FormatField(const Storage<R, P> &storage, std::string_view ctx) {
    return FormatField(storage.Count(), ctx);
}

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
        Field field;
        std::string key;
        MemberPtrVar member_ptr_var;
        std::string suffix{};
        std::string parse_ctx{};
        std::string format_ctx{};
    };

    explicit KeyValueParser(std::initializer_list<Entry> entries) : field_table_{entries} {}
    KeyValueParser(std::initializer_list<Entry> entries, std::string delim)
        : field_table_{entries}, delim_{std::move(delim)} {}
    KeyValueParser(std::initializer_list<Entry> entries, bool (*stop)(std::string_view))
        : field_table_{entries}, stop_{stop} {}
    KeyValueParser(std::initializer_list<Entry> entries, std::string delim, bool (*stop)(std::string_view))
        : field_table_{entries}, delim_{std::move(delim)}, stop_{stop} {}

    static bool ParseMemberPtrVar(std::string_view s, MemberPtrVar member_ptr_var, Struct &obj, std::string_view ctx) {
        auto f = [s, &obj, ctx](auto member_ptr) {
            auto &member{obj.*member_ptr};
            return ParseField(s, member, ctx);
        };
        return std::visit(f, member_ptr_var);
    }

    static std::string FormatMemberPtrVar(MemberPtrVar member_ptr_var, const Struct &obj, std::string_view ctx) {
        auto f = [&obj, ctx](auto member_ptr) -> std::string {
            auto &member{obj.*member_ptr};
            return FormatField(member, ctx);
        };
        return std::visit(f, member_ptr_var);
    }

    // 根据注册表向object中各字段填值
    EnumBitset<Field> Parse(std::istream &is, Struct &object, const EnumBitset<Field> &field_mask) {
        EnumBitset<Field> parsed;
        std::streampos checkpoint{is.tellg()};
        std::string line_buf;
        while (std::getline(is, line_buf)) {
            std::string_view line{line_buf};
            if (stop_ && stop_(line)) {
                is.seekg(checkpoint);
                break;
            }
            checkpoint = is.tellg();
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
            if (parsed.Test(it->field)) {
                continue; // 已解析过
            }

            std::string_view value{Strip(line.substr(pos + 1))};
            if (ParseMemberPtrVar(value, it->member_ptr_var, object, it->parse_ctx)) {
                parsed |= it->field;
            } else {
                std::cerr << "Error: parse field '" << it->key << "' with value '" << value << "' failed" << std::endl;
            }
            if (parsed == field_mask) {
                break; // 已解析全量
            }
        }
        if (stop_) {
            while (std::getline(is, line_buf)) {
                if (stop_(line_buf)) {
                    is.seekg(checkpoint);
                    break;
                }
                checkpoint = is.tellg();
            }
        }
        return parsed;
    }

    void Format(std::ostream &os, const Struct &object, const EnumBitset<Field> &field_mask = ~EnumBitset<Field>{}) {
        for (const auto &entry : field_table_) {
            if (field_mask.Test(entry.field)) {
                std::string value{FormatMemberPtrVar(entry.member_ptr_var, object, entry.format_ctx)};
                os << entry.key << delim_ << ' ' << value;
                if (!entry.suffix.empty()) {
                    os << ' ' << entry.suffix;
                }
                os << '\n';
            }
        }
    }

private:
    std::vector<Entry> field_table_;
    std::string delim_{":"};
    bool (*stop_)(std::string_view){nullptr};
};
} // namespace oops
