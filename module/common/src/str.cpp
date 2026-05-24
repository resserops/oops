#include "oops/str.h"

#include <cassert>
#include <cstring>

namespace oops {
std::string Repeat(const std::string &str, std::size_t n) {
    if (n == 0 || str.empty()) {
        return {};
    }
    std::string res;
    res.reserve(n * str.size());
    for (std::size_t i{0}; i < n; ++i) {
        res.append(str);
    }
    return res;
}

std::string SplitBack(const std::string &str, const std::string &delim) {
    return str.substr(str.rfind(delim) + delim.size());
}

void SplitView::Iterator::NextToken() {
    if (s_.data() == nullptr) {
        return;
    }

    do {
        if (s_.empty()) {
            s_ = {};
            return;
        }

        std::size_t pos;
        if (any_of_delims_) {
            pos = s_.find_first_of(delim_);
        } else if (delim_.empty()) {
            pos = 1;
        } else {
            pos = s_.find(delim_);
        }

        if (pos == std::string_view::npos) {
            token_ = s_;
            s_.remove_prefix(s_.size());
        } else {
            token_ = s_.substr(0, pos);
            if (any_of_delims_) {
                s_.remove_prefix(pos + 1);
            } else {
                s_.remove_prefix(pos + delim_.size());
            }
        }
    } while (skip_empty_ && token_.empty() && s_.data() != nullptr);
}

std::string ToLower(std::string_view s) noexcept {
    std::string res;
    res.reserve(s.size());
    for (char ch : s) {
        res.push_back(ToLower(ch));
    }
    return res;
}

std::string Elide(std::string_view s, std::size_t n) {
    if (s.size() <= n) {
        return std::string{s};
    }
    if (n <= 3) {
        return std::string(n, '.');
    }
    return std::string{s.substr(0, n - 3)} + "...";
}
} // namespace oops
