#include "oops/enum_bitset.h"
#include "gtest/gtest.h"

using namespace oops;

TEST(CommonEnumBitset, EnumBitset) {
  enum class A { A1, A2, COUNT };

  constexpr EnumBitset<A> a{std::bitset<2>{}};
}
