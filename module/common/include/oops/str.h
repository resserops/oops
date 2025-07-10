#pragma once

#include <cstddef>
#include <string>

namespace oops {
std::string StrRepeat(const std::string &str, size_t n);
std::string operator*(const std::string &str, size_t n);
std::string operator*(size_t n, const std::string &str);

std::string StrSplitBack(const std::string &str, const std::string &delim);

template <typename Iter>
void StrSplitToIter(const std::string &str, Iter iter);
template <typename Iter>
void StrSplitToIter(const std::string &str, const std::string &delim, Iter iter);
template <typename Iter>
void StrSplitToIter(const std::string &str, const std::string &delim, bool skip_empty, Iter iter);

template <typename Iter>
void StrSplitToIterMultiDelim(const std::string &str, const std::string &delims, bool skip_empty, Iter iter);
template <typename Iter>
void StrSplitToIterMultiDelim(const std::string &str, const std::string &delims, Iter iter);

template <typename T>
std::string ToStr(const T &t);
}

#include "oops/str.tpp"
