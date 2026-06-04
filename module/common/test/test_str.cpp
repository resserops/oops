#include <type_traits>

#include "oops/str.h"
#include "oops/view.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

#define TEST_STATIC(suite, case) constexpr void TestStatic_##suite_##case ()
using namespace testing;

static std::string FmtChar(char c) {
    int c_int{static_cast<unsigned char>(c)};
    std::stringstream ss;
    ss << "Char: 0x" << std::hex << c_int;
    if (std::isprint(c_int)) {
        ss << " '" << c << "'";
    }
    return ss.str();
}

TEST_STATIC(CommonStr, IsUpper) {
    static_assert(oops::IsUpper('A'));
    static_assert(oops::IsUpper('Z'));

    static_assert(!oops::IsUpper('a'));
    static_assert(!oops::IsUpper('z'));
    static_assert(!oops::IsUpper('@'));
    static_assert(!oops::IsUpper('['));

    static_assert(!oops::IsUpper('0'));
    static_assert(!oops::IsUpper(' '));
    static_assert(!oops::IsUpper('\0'));
    static_assert(!oops::IsUpper('\n'));
}

TEST(CommonStr, IsUpperSameAsStd) {
    for (char c : oops::view::IntegerSet<char>) {
        int c_int{static_cast<unsigned char>(c)};
        EXPECT_EQ(oops::IsUpper(c), std::isupper(c_int) != 0) << FmtChar(c);
    }
}

TEST_STATIC(CommonStr, IsLower) {
    static_assert(oops::IsLower('a'));
    static_assert(oops::IsLower('z'));

    static_assert(!oops::IsLower('A'));
    static_assert(!oops::IsLower('Z'));
    static_assert(!oops::IsLower('`'));
    static_assert(!oops::IsLower('{'));

    static_assert(!oops::IsLower('0'));
    static_assert(!oops::IsLower(' '));
    static_assert(!oops::IsLower('\0'));
    static_assert(!oops::IsLower('\n'));
}

TEST(CommonStr, IsLowerSameAsStd) {
    for (char c : oops::view::IntegerSet<char>) {
        int c_int{static_cast<unsigned char>(c)};
        EXPECT_EQ(oops::IsLower(c), std::islower(c_int) != 0) << FmtChar(c);
    }
}

TEST_STATIC(CommonStr, IsAlpha) {
    static_assert(oops::IsAlpha('A'));
    static_assert(oops::IsAlpha('Z'));
    static_assert(oops::IsAlpha('a'));
    static_assert(oops::IsAlpha('z'));

    static_assert(!oops::IsAlpha('@'));
    static_assert(!oops::IsAlpha('['));
    static_assert(!oops::IsAlpha('`'));
    static_assert(!oops::IsAlpha('{'));

    static_assert(!oops::IsAlpha('0'));
    static_assert(!oops::IsAlpha(' '));
    static_assert(!oops::IsAlpha('\0'));
    static_assert(!oops::IsAlpha('\n'));
}

TEST(CommonStr, IsAlphaSameAsStd) {
    for (char c : oops::view::IntegerSet<char>) {
        int c_int{static_cast<unsigned char>(c)};
        EXPECT_EQ(oops::IsAlpha(c), std::isalpha(c_int) != 0) << FmtChar(c);
    }
}

TEST_STATIC(CommonStr, IsDigit) {
    static_assert(oops::IsDigit('0'));
    static_assert(oops::IsDigit('9'));

    static_assert(!oops::IsDigit('/'));
    static_assert(!oops::IsDigit(':'));

    static_assert(!oops::IsDigit('A'));
    static_assert(!oops::IsDigit('a'));
    static_assert(!oops::IsDigit(' '));
    static_assert(!oops::IsDigit('\0'));
    static_assert(!oops::IsDigit('\n'));
}

TEST(CommonStr, IsDigitSameAsStd) {
    for (char c : oops::view::IntegerSet<char>) {
        int c_int{static_cast<unsigned char>(c)};
        EXPECT_EQ(oops::IsDigit(c), std::isdigit(c_int) != 0) << FmtChar(c);
    }
}

TEST_STATIC(CommonStr, IsAlnum) {
    static_assert(oops::IsAlnum('A'));
    static_assert(oops::IsAlnum('Z'));
    static_assert(oops::IsAlnum('a'));
    static_assert(oops::IsAlnum('z'));
    static_assert(oops::IsAlnum('0'));
    static_assert(oops::IsAlnum('9'));

    static_assert(!oops::IsAlnum('@'));
    static_assert(!oops::IsAlnum('['));
    static_assert(!oops::IsAlnum('`'));
    static_assert(!oops::IsAlnum('{'));
    static_assert(!oops::IsAlnum('/'));
    static_assert(!oops::IsAlnum(':'));

    static_assert(!oops::IsAlnum(' '));
    static_assert(!oops::IsAlnum('\0'));
    static_assert(!oops::IsAlnum('\n'));
}

TEST(CommonStr, IsAlnumSameAsStd) {
    for (char c : oops::view::IntegerSet<char>) {
        int c_int{static_cast<unsigned char>(c)};
        EXPECT_EQ(oops::IsAlnum(c), std::isalnum(c_int) != 0) << FmtChar(c);
    }
}

TEST_STATIC(CommonStr, ToLower) {
    static_assert(oops::ToLower('A') == 'a');
    static_assert(oops::ToLower('Z') == 'z');

    static_assert(oops::ToLower('a') == 'a');
    static_assert(oops::ToLower('z') == 'z');
    static_assert(oops::ToLower('@') == '@');
    static_assert(oops::ToLower('[') == '[');

    static_assert(oops::ToLower('0') == '0');
    static_assert(oops::ToLower(' ') == ' ');
    static_assert(oops::ToLower('\0') == '\0');
    static_assert(oops::ToLower('\n') == '\n');
}

TEST(CommonStr, ToLowerSameAsStd) {
    for (char c : oops::view::IntegerSet<char>) {
        int c_int{static_cast<unsigned char>(c)};
        EXPECT_EQ(oops::ToLower(c), static_cast<char>(std::tolower(c_int))) << FmtChar(c);
    }
}

TEST_STATIC(CommonStr, ToUpper) {
    static_assert(oops::ToUpper('a') == 'A');
    static_assert(oops::ToUpper('z') == 'Z');

    static_assert(oops::ToUpper('A') == 'A');
    static_assert(oops::ToUpper('Z') == 'Z');
    static_assert(oops::ToUpper('`') == '`');
    static_assert(oops::ToUpper('{') == '{');

    static_assert(oops::ToUpper('0') == '0');
    static_assert(oops::ToUpper(' ') == ' ');
    static_assert(oops::ToUpper('\0') == '\0');
    static_assert(oops::ToUpper('\n') == '\n');
}

TEST(CommonStr, ToUpperSameAsStd) {
    for (char c : oops::view::IntegerSet<char>) {
        int c_int{static_cast<unsigned char>(c)};
        EXPECT_EQ(oops::ToUpper(c), static_cast<char>(std::toupper(c_int))) << FmtChar(c);
    }
}

TEST(CommonStr, ToSv) {
    for (char c : oops::view::IntegerSet<char>) {
        std::string_view sv{oops::ToSv(c)};
        EXPECT_EQ(sv.size(), 1);
        EXPECT_EQ(sv.front(), c);
    }
}

TEST(CommonStr, SplitBack) {
    using namespace oops;
    EXPECT_EQ(SplitBack("abc/def/ghi", "/"), "ghi");
    EXPECT_EQ(SplitBack("abc/def/ghi/", "/"), "");
    EXPECT_EQ(SplitBack("/", "/"), "");
    EXPECT_EQ(SplitBack("abc", "/"), "abc");
    EXPECT_EQ(SplitBack("", "/"), "");
    EXPECT_EQ(SplitBack("abc", ""), "");
}

TEST(CommonStr, Split) {
    EXPECT_THAT(oops::Split(""), ElementsAre());
    EXPECT_THAT(oops::Split(" "), ElementsAre());
    EXPECT_THAT(oops::Split("  "), ElementsAre());
    EXPECT_THAT(oops::Split("abc"), ElementsAre("abc"));
    EXPECT_THAT(oops::Split("abc "), ElementsAre("abc"));
    EXPECT_THAT(oops::Split("abc  "), ElementsAre("abc"));
    EXPECT_THAT(oops::Split(" abc"), ElementsAre("abc"));
    EXPECT_THAT(oops::Split("  abc"), ElementsAre("abc"));
    EXPECT_THAT(oops::Split("abc def"), ElementsAre("abc", "def"));
    EXPECT_THAT(oops::Split("abc  def"), ElementsAre("abc", "def"));
    EXPECT_THAT(oops::Split("abc def ghi"), ElementsAre("abc", "def", "ghi"));
}

TEST(CommonStr, SplitWhitespace) {
    EXPECT_THAT(oops::Split("\t"), ElementsAre());
    EXPECT_THAT(oops::Split("\n\r"), ElementsAre());
    EXPECT_THAT(oops::Split("abc\v"), ElementsAre("abc"));
    EXPECT_THAT(oops::Split("abc\f\t"), ElementsAre("abc"));
    EXPECT_THAT(oops::Split("\nabc"), ElementsAre("abc"));
    EXPECT_THAT(oops::Split("\r\vabc"), ElementsAre("abc"));
    EXPECT_THAT(oops::Split("abc\fdef"), ElementsAre("abc", "def"));
    EXPECT_THAT(oops::Split("abc\t\ndef"), ElementsAre("abc", "def"));
    EXPECT_THAT(oops::Split("abc\rdef\vghi"), ElementsAre("abc", "def", "ghi"));
}

TEST(CommonStr, SplitDelim) {
    EXPECT_THAT(oops::Split("", ":"), ElementsAre(""));
    EXPECT_THAT(oops::Split(":", ":"), ElementsAre("", ""));
    EXPECT_THAT(oops::Split("::", ":"), ElementsAre("", "", ""));
    EXPECT_THAT(oops::Split("abc", ":"), ElementsAre("abc"));
    EXPECT_THAT(oops::Split("abc:", ":"), ElementsAre("abc", ""));
    EXPECT_THAT(oops::Split("abc::", ":"), ElementsAre("abc", "", ""));
    EXPECT_THAT(oops::Split(":abc", ":"), ElementsAre("", "abc"));
    EXPECT_THAT(oops::Split("::abc", ":"), ElementsAre("", "", "abc"));
    EXPECT_THAT(oops::Split("abc:def", ":"), ElementsAre("abc", "def"));
    EXPECT_THAT(oops::Split("abc::def", ":"), ElementsAre("abc", "", "def"));
    EXPECT_THAT(oops::Split("abc:def:ghi", ":"), ElementsAre("abc", "def", "ghi"));
}

TEST(CommonStr, SplitCharDelim) {
    EXPECT_THAT(oops::Split("", ':'), ElementsAre(""));
    EXPECT_THAT(oops::Split(":", ':'), ElementsAre("", ""));
    EXPECT_THAT(oops::Split("::", ':'), ElementsAre("", "", ""));
    EXPECT_THAT(oops::Split("abc", ':'), ElementsAre("abc"));
    EXPECT_THAT(oops::Split("abc:", ':'), ElementsAre("abc", ""));
    EXPECT_THAT(oops::Split("abc::", ':'), ElementsAre("abc", "", ""));
    EXPECT_THAT(oops::Split(":abc", ':'), ElementsAre("", "abc"));
    EXPECT_THAT(oops::Split("::abc", ':'), ElementsAre("", "", "abc"));
    EXPECT_THAT(oops::Split("abc:def", ':'), ElementsAre("abc", "def"));
    EXPECT_THAT(oops::Split("abc::def", ':'), ElementsAre("abc", "", "def"));
    EXPECT_THAT(oops::Split("abc:def:ghi", ':'), ElementsAre("abc", "def", "ghi"));
}

TEST(CommonStr, SplitMultiDelims) {
    EXPECT_THAT(oops::Split("", "::"), ElementsAre(""));
    EXPECT_THAT(oops::Split(":", "::"), ElementsAre(":"));
    EXPECT_THAT(oops::Split("::", "::"), ElementsAre("", ""));
    EXPECT_THAT(oops::Split(":::", "::"), ElementsAre("", ":"));
    EXPECT_THAT(oops::Split("::::", "::"), ElementsAre("", "", ""));
    EXPECT_THAT(oops::Split("abc", "::"), ElementsAre("abc"));
    EXPECT_THAT(oops::Split("abc:", "::"), ElementsAre("abc:"));
    EXPECT_THAT(oops::Split("abc::", "::"), ElementsAre("abc", ""));
    EXPECT_THAT(oops::Split("abc:::", "::"), ElementsAre("abc", ":"));
    EXPECT_THAT(oops::Split("abc::::", "::"), ElementsAre("abc", "", ""));
    EXPECT_THAT(oops::Split(":abc", "::"), ElementsAre(":abc"));
    EXPECT_THAT(oops::Split("::abc", "::"), ElementsAre("", "abc"));
    EXPECT_THAT(oops::Split(":::abc", "::"), ElementsAre("", ":abc"));
    EXPECT_THAT(oops::Split("::::abc", "::"), ElementsAre("", "", "abc"));
    EXPECT_THAT(oops::Split("abc:def", "::"), ElementsAre("abc:def"));
    EXPECT_THAT(oops::Split("abc::def", "::"), ElementsAre("abc", "def"));
    EXPECT_THAT(oops::Split("abc:::def", "::"), ElementsAre("abc", ":def"));
    EXPECT_THAT(oops::Split("abc::::def", "::"), ElementsAre("abc", "", "def"));
    EXPECT_THAT(oops::Split("abc::def::ghi", "::"), ElementsAre("abc", "def", "ghi"));
}

TEST(CommonStr, SplitAnyOfDelims) {
    EXPECT_THAT(oops::Split("", ":;").AnyOfDelims(), ElementsAre(""));
    EXPECT_THAT(oops::Split(":", ":;").AnyOfDelims(), ElementsAre("", ""));
    EXPECT_THAT(oops::Split(";", ":;").AnyOfDelims(), ElementsAre("", ""));
    EXPECT_THAT(oops::Split(":;", ":;").AnyOfDelims(), ElementsAre("", "", ""));
    EXPECT_THAT(oops::Split("abc", ":;").AnyOfDelims(), ElementsAre("abc"));
    EXPECT_THAT(oops::Split("abc:", ":;").AnyOfDelims(), ElementsAre("abc", ""));
    EXPECT_THAT(oops::Split("abc;", ":;").AnyOfDelims(), ElementsAre("abc", ""));
    EXPECT_THAT(oops::Split("abc:;", ":;").AnyOfDelims(), ElementsAre("abc", "", ""));
    EXPECT_THAT(oops::Split(":abc", ":;").AnyOfDelims(), ElementsAre("", "abc"));
    EXPECT_THAT(oops::Split(";abc", ":;").AnyOfDelims(), ElementsAre("", "abc"));
    EXPECT_THAT(oops::Split(":;abc", ":;").AnyOfDelims(), ElementsAre("", "", "abc"));
    EXPECT_THAT(oops::Split("abc:def", ":;").AnyOfDelims(), ElementsAre("abc", "def"));
    EXPECT_THAT(oops::Split("abc;def", ":;").AnyOfDelims(), ElementsAre("abc", "def"));
    EXPECT_THAT(oops::Split("abc:;def", ":;").AnyOfDelims(), ElementsAre("abc", "", "def"));
    EXPECT_THAT(oops::Split("abc:def;ghi", ":;").AnyOfDelims(), ElementsAre("abc", "def", "ghi"));
}

TEST(CommonStr, SplitNone) {
    // 跟随std::views::split行为，做逐字符分割
    EXPECT_THAT(oops::Split("", ""), ElementsAre(""));
    EXPECT_THAT(oops::Split("a", ""), ElementsAre("a"));
    EXPECT_THAT(oops::Split("ab", ""), ElementsAre("a", "b"));
    EXPECT_THAT(oops::Split("abc", ""), ElementsAre("a", "b", "c"));
}

TEST(CommonStr, SplitAnyOfNone) {
    EXPECT_THAT(oops::Split("", "").AnyOfDelims(), ElementsAre(""));
    EXPECT_THAT(oops::Split("a", "").AnyOfDelims(), ElementsAre("a"));
    EXPECT_THAT(oops::Split("ab", "").AnyOfDelims(), ElementsAre("ab"));
    EXPECT_THAT(oops::Split("abc", "").AnyOfDelims(), ElementsAre("abc"));
}

TEST(CommonStr, SplitNotSkipEmpty) {
    EXPECT_THAT(oops::Split("").SkipEmpty(false), ElementsAre(""));
    EXPECT_THAT(oops::Split(" ").SkipEmpty(false), ElementsAre("", ""));
    EXPECT_THAT(oops::Split("  ").SkipEmpty(false), ElementsAre("", "", ""));
    EXPECT_THAT(oops::Split("abc").SkipEmpty(false), ElementsAre("abc"));
    EXPECT_THAT(oops::Split("abc ").SkipEmpty(false), ElementsAre("abc", ""));
    EXPECT_THAT(oops::Split("abc  ").SkipEmpty(false), ElementsAre("abc", "", ""));
    EXPECT_THAT(oops::Split(" abc").SkipEmpty(false), ElementsAre("", "abc"));
    EXPECT_THAT(oops::Split("  abc").SkipEmpty(false), ElementsAre("", "", "abc"));
    EXPECT_THAT(oops::Split("abc def").SkipEmpty(false), ElementsAre("abc", "def"));
    EXPECT_THAT(oops::Split("abc  def").SkipEmpty(false), ElementsAre("abc", "", "def"));
    EXPECT_THAT(oops::Split("abc def ghi").SkipEmpty(false), ElementsAre("abc", "def", "ghi"));
}

TEST(CommonStr, SplitDelimSkipEmpty) {
    EXPECT_THAT(oops::Split("", ":").SkipEmpty(), ElementsAre());
    EXPECT_THAT(oops::Split(":", ":").SkipEmpty(), ElementsAre());
    EXPECT_THAT(oops::Split("::", ":").SkipEmpty(), ElementsAre());
    EXPECT_THAT(oops::Split("abc", ":").SkipEmpty(), ElementsAre("abc"));
    EXPECT_THAT(oops::Split("abc:", ":").SkipEmpty(), ElementsAre("abc"));
    EXPECT_THAT(oops::Split("abc::", ":").SkipEmpty(), ElementsAre("abc"));
    EXPECT_THAT(oops::Split(":abc", ":").SkipEmpty(), ElementsAre("abc"));
    EXPECT_THAT(oops::Split("::abc", ":").SkipEmpty(), ElementsAre("abc"));
    EXPECT_THAT(oops::Split("abc:def", ":").SkipEmpty(), ElementsAre("abc", "def"));
    EXPECT_THAT(oops::Split("abc::def", ":").SkipEmpty(), ElementsAre("abc", "def"));
    EXPECT_THAT(oops::Split("abc:def:ghi", ":").SkipEmpty(), ElementsAre("abc", "def", "ghi"));
}

TEST(CommonStr, Repeat) {
    using namespace oops;
    EXPECT_EQ(Repeat("abc", 0), "");
    EXPECT_EQ(Repeat("abc", 1), "abc");
    EXPECT_EQ(Repeat("abc", 3), "abcabcabc");
    EXPECT_EQ(Repeat("", 0), "");
    EXPECT_EQ(Repeat("", 1), "");
    EXPECT_EQ(Repeat("", 3), "");
}

TEST(CommonStr, StartsWith) {
    using namespace oops;
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
    using namespace oops;
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
    using namespace oops;
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
