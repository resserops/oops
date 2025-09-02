#include <type_traits>

#include "gtest/gtest.h"
#include "oops/str.h"

using namespace oops;
using namespace oops::str;
using namespace std::literals;

TEST(CommonStr, StrRepeat) {
    EXPECT_EQ(StrRepeat("abc", 0), "");
    EXPECT_EQ(StrRepeat("abc", 1), "abc");
    EXPECT_EQ(StrRepeat("abc", 3), "abcabcabc");
    EXPECT_EQ(StrRepeat("", 0), "");
    EXPECT_EQ(StrRepeat("", 1), "");
    EXPECT_EQ(StrRepeat("", 3), "");
}

TEST(CommonStr, StrMul) {
    EXPECT_EQ("abc"s * 0, "");
    EXPECT_EQ("abc"s * 1, "abc");
    EXPECT_EQ("abc"s * 3, "abcabcabc");
    EXPECT_EQ(0 * "abc"s, "");
    EXPECT_EQ(1 * "abc"s, "abc");
    EXPECT_EQ(3 * "abc"s, "abcabcabc");
}

TEST(CommonStr, StrSplitBack) {
    EXPECT_EQ(StrSplitBack("abc/def/ghi", "/"), "ghi");
    EXPECT_EQ(StrSplitBack("abc/def/ghi/", "/"), "");
    EXPECT_EQ(StrSplitBack("/", "/"), "");
    EXPECT_EQ(StrSplitBack("abc", "/"), "abc");
    EXPECT_EQ(StrSplitBack("", "/"), "");
    EXPECT_EQ(StrSplitBack("abc", ""), "");
}

TEST(CommonStr, StrSplitToIter) {
    std::vector<std::string> res;
    StrSplitToIter("abc defg hi", std::back_inserter(res));
    EXPECT_EQ(res, (std::vector<std::string>{"abc", "defg", "hi"}));

    res.clear();
    EXPECT_TRUE(res.empty());
    StrSplitToIter(" abc   defg hi  ", std::back_inserter(res));
    EXPECT_EQ(res, (std::vector<std::string>{"abc", "defg", "hi"}));

    res.clear();
    StrSplitToIter("\nabc \t defg\fhi  ", std::back_inserter(res));
    EXPECT_EQ(res, (std::vector<std::string>{"abc", "defg", "hi"}));
}

TEST(CommonStr, StrSplitToIterDelim) {
    std::vector<std::string> res;
    StrSplitToIter("abc:defg:hi", ":", std::back_inserter(res));
    EXPECT_EQ(res, (std::vector<std::string>{"abc", "defg", "hi"}));
}

TEST(CommonStr, StartsWith) {
    EXPECT_TRUE(StartsWith("abc", ""));
    EXPECT_TRUE(StartsWith("abc", "a"));
    EXPECT_TRUE(StartsWith("abc", "ab"));
    EXPECT_TRUE(StartsWith("abc", "abc"));
    EXPECT_FALSE(StartsWith("abc", "d"));
    EXPECT_FALSE(StartsWith("abc", "ad"));
    EXPECT_FALSE(StartsWith("abc", "abd"));
    EXPECT_FALSE(StartsWith("abc", "abcd"));
}

TEST(CommonStr, EndsWith) {
    EXPECT_TRUE(EndsWith("abc", ""));
    EXPECT_TRUE(EndsWith("abc", "c"));
    EXPECT_TRUE(EndsWith("abc", "bc"));
    EXPECT_TRUE(EndsWith("abc", "abc"));
    EXPECT_FALSE(EndsWith("abc", "d"));
    EXPECT_FALSE(EndsWith("abc", "dc"));
    EXPECT_FALSE(EndsWith("abc", "dbc"));
    EXPECT_FALSE(EndsWith("abc", "dabc"));
}

TEST(CommonStr, Strip) {
    EXPECT_EQ(Strip(""), "");
    EXPECT_EQ(Strip(" "), "");
    EXPECT_EQ(Strip("  "), "");
    EXPECT_EQ(Strip("a"), "a");
    EXPECT_EQ(Strip(" a"), "a");
    EXPECT_EQ(Strip("a "), "a");
    EXPECT_EQ(Strip(" a "), "a");
    EXPECT_EQ(Strip("  a"), "a");
    EXPECT_EQ(Strip("a  "), "a");
    EXPECT_EQ(Strip("  a  "), "a");
    EXPECT_EQ(Strip("abc"), "abc");
    EXPECT_EQ(Strip(" abc"), "abc");
    EXPECT_EQ(Strip("abc "), "abc");
    EXPECT_EQ(Strip(" abc "), "abc");
    EXPECT_EQ(Strip("  abc"), "abc");
    EXPECT_EQ(Strip("abc  "), "abc");
    EXPECT_EQ(Strip("  abc  "), "abc");
    EXPECT_EQ(Strip("  a b c  "), "a b c");
}