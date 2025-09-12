#pragma once

#include <iterator>
#include <utility>

namespace oops {
template <typename T> class ReverseView {
public:
  explicit ReverseView(T &&t) : t_{std::forward<T>(t)} {}

  auto begin() { return ::std::rbegin(t_); }
  auto end() { return ::std::rend(t_); }

  auto begin() const { return ::std::crbegin(t_); }
  auto end() const { return ::std::crend(t_); }

  auto cbegin() const { return ::std::crbegin(t_); }
  auto cend() const { return ::std::crend(t_); }

  auto rbegin() { return ::std::begin(t_); }
  auto rend() { return ::std::end(t_); }

  auto rbegin() const { return ::std::cbegin(t_); }
  auto rend() const { return ::std::cend(t_); }

  auto crbegin() const { return ::std::cbegin(t_); }
  auto crend() const { return ::std::cend(t_); }

private:
  T t_;
};

template <typename T> auto ReverseRange(T &&t) {
  return ReverseView<T>{std::forward<T>(t)};
}
} // namespace oops
