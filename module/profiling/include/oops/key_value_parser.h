#pragma once
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <istream>
#include <ostream>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
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

// std::vector<T>特化：空格分隔token序列，ctx逐元素透传
template <typename T>
bool ParseVector(std::string_view s, std::vector<T> &v, std::string_view ctx) {
    v.clear();
    for (auto token : Split(s)) {
        T value{};
        if (!ParseField(token, value, ctx)) {
            return false;
        }
        v.push_back(std::move(value));
    }
    return true;
}

template <typename T>
std::string FormatVector(const std::vector<T> &v, std::string_view ctx) {
    std::string s;
    s.reserve(64); // one cache line
    for (const auto &item : v) {
        s += FormatField(item, ctx);
        s += ' ';
    }
    if (!s.empty()) {
        s.pop_back();
    }
    return s;
}

template <typename T>
bool ParseField(std::string_view s, std::vector<T> &v, std::string_view ctx) {
    return ParseVector(s, v, ctx);
}

template <typename T>
std::string FormatField(const std::vector<T> &v, std::string_view ctx) {
    return FormatVector(v, ctx);
}

// std::vector<bool>特化：
// ctx = "{:pb}"，按Linux内核%pb格式解析/格式化
// ctx = other，按默认std::vector<T>格式解析/格式化
template <>
inline bool ParseField(std::string_view s, std::vector<bool> &v, std::string_view ctx) {
    if (ctx != "%*pb") {
        return ParseVector(s, v, ctx);
    }

    std::vector<std::uint32_t> chunks; // 高位组在前
    std::size_t digits{0};
    for (auto token : Split(s, ',')) {
        token = Strip(token);
        std::uint32_t chunk{};
        if (token.empty() || (!chunks.empty() && token.size() != 8) || !ParseField(token, chunk, "{:x}")) {
            return false; // 除最高组外每组固定8字符
        }
        chunks.push_back(chunk);
        digits += token.size();
    }

    v.assign(digits * 4, false);
    for (std::size_t c{0}; c < chunks.size(); ++c) {
        for (std::size_t b{0}; b < 32; ++b) {
            if (chunks[c] & (1u << b)) {
                v[(chunks.size() - 1 - c) * 32 + b] = true; // 首个chunk为最高组
            }
        }
    }
    return true;
}

template <>
inline std::string FormatField(const std::vector<bool> &v, std::string_view ctx) {
    if (ctx != "%*pb") {
        return FormatVector(v, ctx);
    }

    const std::size_t digits{(v.size() + 3) / 4};
    const std::size_t chunks{(digits + 7) / 8};
    std::string s;
    s.reserve(digits + chunks - 1);
    for (std::size_t c{chunks}; c-- > 0;) {
        std::uint32_t chunk{};
        for (std::size_t b{0}; b < 32 && c * 32 + b < v.size(); ++b) {
            if (v[c * 32 + b]) {
                chunk |= 1u << b;
            }
        }
        // 除最高组固定8字符外，最高组宽度为剩余16进制位数
        s += fmt::format("{:0{}x}", chunk, c == chunks - 1 ? digits - 8 * (chunks - 1) : 8);
        if (c > 0) {
            s += ',';
        }
    }
    return s;
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
        Entry(Field f, std::string k, MemberPtrVar m) : field{f}, key{std::move(k)}, member_ptr_var{m} {}
        Entry(Field f, std::string k, MemberPtrVar m, std::string ctx)
            : field{f}, key{std::move(k)}, member_ptr_var{m}, parse_ctx{ctx}, format_ctx{std::move(ctx)} {}
        Entry(Field f, std::string k, MemberPtrVar m, std::string pctx, std::string fctx)
            : field{f}, key{std::move(k)}, member_ptr_var{m}, parse_ctx{std::move(pctx)}, format_ctx{std::move(fctx)} {}

        Field field;
        std::string key;
        MemberPtrVar member_ptr_var;
        std::string parse_ctx;
        std::string format_ctx;
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
                os << entry.key << delim_ << ' ' << value << '\n';
            }
        }
    }

private:
    std::vector<Entry> field_table_;
    std::string delim_{":"};
    bool (*stop_)(std::string_view){nullptr};
};
} // namespace oops
