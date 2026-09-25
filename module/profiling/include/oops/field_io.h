#pragma once
#include <charconv>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "fast_float/fast_float.h"
#include "fmt/format.h"
#include "scn/scan.h"

#include "oops/storage.h"
#include "oops/str.h"

namespace oops {
// 基础路径
template <typename T>
bool ScanField(std::string_view s, T &t) {
    const char *end{s.data() + s.size()};
    auto [ptr, ec]{std::from_chars(s.data(), end, t)};
    return ec == std::errc{};
}

template <typename T>
std::string FormatField(const T &t) {
    return fmt::format("{}", t);
}

// 浮点数重载
inline bool ScanField(std::string_view s, float &f) {
    auto [ptr, ec]{fast_float::from_chars(s.data(), s.data() + s.size(), f)};
    return ec == std::errc{};
}

inline bool ScanField(std::string_view s, double &d) {
    auto [ptr, ec]{fast_float::from_chars(s.data(), s.data() + s.size(), d)};
    return ec == std::errc{};
}

// std::string重载
inline bool ScanField(std::string_view s, std::string &t) {
    t = Strip(s);
    return true;
}

// std::vector<T>重载
template <typename T>
bool ScanField(std::string_view s, std::vector<T> &v) {
    v.clear();
    for (auto token : Split(s)) {
        T value{};
        if (!ScanField(token, value)) {
            return false;
        }
        v.push_back(std::move(value));
    }
    return true;
}

template <typename T>
std::string FormatField(const std::vector<T> &v) {
    std::string s;
    s.reserve(64); // one cache line
    for (const auto &item : v) {
        s += FormatField(item);
        s += ' ';
    }
    if (!s.empty()) {
        s.pop_back();
    }
    return s;
}

// scan/format with context
template <typename T>
bool ScanField(std::string_view s, T &t, std::string_view ctx) {
    auto scan_res{scn::scan<T>(s, ctx)};
    if (scan_res) {
        t = scan_res->value();
    }
    return bool{scan_res};
}

template <typename T>
std::string FormatField(const T &t, std::string_view ctx) {
    return fmt::format(ctx, t);
}

// oops::Storage重载
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
