#pragma once
#include <algorithm>
#include <cstddef>
#include <iostream>
#include <istream>
#include <ostream>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#include "oops/enum_bitset.h"
#include "oops/field_io.h"
#include "oops/str.h"

namespace oops {
template <auto V>
struct ConstantWrapper {
    static constexpr auto value{V};
    constexpr operator decltype(V)() const noexcept { return V; }
};
template <auto V>
inline constexpr ConstantWrapper<V> CW{};

namespace detail {
template <typename Struct, auto Member, auto Scan>
bool BakeScan(std::string_view s, Struct &obj, std::string_view) {
    return Scan(s, obj.*Member);
}

template <typename Struct, auto Member, auto Format>
std::string BakeFormat(const Struct &obj, std::string_view) {
    return Format(obj.*Member);
}

template <typename Struct, auto Member>
bool BakeScan(std::string_view s, Struct &obj, std::string_view ctx) {
    return ScanField(s, obj.*Member, ctx);
}

template <typename Struct, auto Member>
std::string BakeFormat(const Struct &obj, std::string_view ctx) {
    return FormatField(obj.*Member, ctx);
}
} // namespace detail

template <typename S, typename F>
class KeyValueParser {
public:
    static_assert(std::is_enum_v<F>);

    using Struct = S;
    using Field = F;

    struct Entry {
        template <std::size_t N, auto Member, auto Scan, auto Format>
        Entry(Field f, const char (&k)[N], ConstantWrapper<Member>, ConstantWrapper<Scan>, ConstantWrapper<Format>)
            : field{f}, key{k, N - 1}, scan{&detail::BakeScan<Struct, Member, Scan>},
              format{&detail::BakeFormat<Struct, Member, Format>} {}

        template <std::size_t N, auto Member>
        Entry(Field f, const char (&k)[N], ConstantWrapper<Member>)
            : field{f}, key{k, N - 1}, scan{&detail::BakeScan<Struct, Member>},
              format{&detail::BakeFormat<Struct, Member>} {}

        template <std::size_t N, auto Member, std::size_t N2>
        Entry(Field f, const char (&k)[N], ConstantWrapper<Member>, const char (&ctx)[N2])
            : field{f}, key{k, N - 1}, scan{&detail::BakeScan<Struct, Member>},
              format{&detail::BakeFormat<Struct, Member>}, scan_ctx{ctx, N2 - 1}, format_ctx{ctx, N2 - 1} {}

        template <std::size_t N, auto Member, std::size_t N2, std::size_t N3>
        Entry(Field f, const char (&k)[N], ConstantWrapper<Member>, const char (&pctx)[N2], const char (&fctx)[N3])
            : field{f}, key{k, N - 1}, scan{&detail::BakeScan<Struct, Member>},
              format{&detail::BakeFormat<Struct, Member>}, scan_ctx{pctx, N2 - 1}, format_ctx{fctx, N3 - 1} {}

        Field field;
        std::string_view key;
        bool (*scan)(std::string_view, Struct &, std::string_view);
        std::string (*format)(const Struct &, std::string_view);
        std::string_view scan_ctx{};
        std::string_view format_ctx{};
    };

    explicit KeyValueParser(std::initializer_list<Entry> entries) : field_table_{entries} {}
    KeyValueParser(std::initializer_list<Entry> entries, std::string delim)
        : field_table_{entries}, delim_{std::move(delim)} {}
    KeyValueParser(std::initializer_list<Entry> entries, bool (*stop)(std::string_view))
        : field_table_{entries}, stop_{stop} {}
    KeyValueParser(std::initializer_list<Entry> entries, std::string delim, bool (*stop)(std::string_view))
        : field_table_{entries}, delim_{std::move(delim)}, stop_{stop} {}

    EnumBitset<Field> Parse(std::istream &is, Struct &obj, const EnumBitset<Field> &field_mask) {
        EnumBitset<Field> parsed;
        std::streampos line_start{is.tellg()};
        std::string buf;
        while (std::getline(is, buf)) {
            std::string_view line{buf};
            if (stop_ && stop_(line)) {
                is.seekg(line_start);
                break;
            }
            line_start = is.tellg();
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
            if (it->scan(value, obj, it->scan_ctx)) {
                parsed |= it->field;
            } else {
                std::cerr << "Error: parse field '" << it->key << "' with value '" << value << "' failed" << std::endl;
            }
            if (parsed == field_mask) {
                break; // 已解析全量
            }
        }
        if (stop_) {
            while (std::getline(is, buf)) {
                if (stop_(buf)) {
                    is.seekg(line_start);
                    break;
                }
                line_start = is.tellg();
            }
        }
        return parsed;
    }

    void Format(std::ostream &os, const Struct &obj, const EnumBitset<Field> &field_mask = ~EnumBitset<Field>{}) {
        std::size_t key_width{0};
        for (const auto &entry : field_table_) {
            if (field_mask.Test(entry.field)) {
                key_width = std::max(key_width, entry.key.size());
            }
        }
        for (const auto &entry : field_table_) {
            if (field_mask.Test(entry.field)) {
                std::string value{entry.format(obj, entry.format_ctx)};
                os << entry.key << Repeat(" ", key_width - entry.key.size()) << delim_ << ' ' << value << '\n';
            }
        }
    }

private:
    std::vector<Entry> field_table_;
    std::string delim_{":"};
    bool (*stop_)(std::string_view){nullptr};
};
} // namespace oops
