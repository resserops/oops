#include "gtest/gtest.h"
#include "oops/str.h"

using namespace oops;

TEST(CommonStr, RepeatStr) {
    EXPECT_EQ(RepeatStr("abc", 0), "");
    EXPECT_EQ(RepeatStr("abc", 1), "abc");
    EXPECT_EQ(RepeatStr("abc", 3), "abcabcabc");
}
