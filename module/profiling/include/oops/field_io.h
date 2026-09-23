#pragma once
#include <cstddef>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "fmt/format.h"
#include "scn/scan.h"

#include "oops/storage.h"
#include "oops/str.h"

namespace oops {
template <typename T>
bool ScanField(std::string_view s, T &t, std::string_view ctx) {
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

// std::string特化：忽略ctx，整个字符串即值
template <>
inline bool ScanField(std::string_view s, std::string &t, std::string_view) {
    t = s;
    return true;
}

template <>
inline std::string FormatField(const std::string &t, std::string_view) {
    return t;
}

// bool特化：支持在ctx中定义代表真假的字符串，例如"true/false"或"yes/no"
template <>
inline bool ScanField(std::string_view s, bool &b, std::string_view ctx) {
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
bool ScanField(std::string_view s, std::vector<T> &v, std::string_view ctx) {
    v.clear();
    for (auto token : Split(s)) {
        T value{};
        if (!ScanField(token, value, ctx)) {
            return false;
        }
        v.push_back(std::move(value));
    }
    return true;
}

template <typename T>
std::string FormatField(const std::vector<T> &v, std::string_view ctx) {
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

// Storage特化，ctx透传至R
template <typename R, typename P>
bool ScanField(std::string_view s, Storage<R, P> &storage, std::string_view ctx) {
    R r{};
    auto res{ScanField<R>(s, r, ctx)};
    storage = Storage<R, P>{r};
    return res;
}

template <typename R, typename P>
std::string FormatField(const Storage<R, P> &storage, std::string_view ctx) {
    return FormatField(storage.Count(), ctx);
}
} // namespace oops
