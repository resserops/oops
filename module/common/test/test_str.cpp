#include <type_traits>

#include "oops/str.h"
#include "oops/view.h"
#include "gtest/gtest.h"

auto CHAR_SET{
    oops::Iota<int>(std::numeric_limits<char>::min(), static_cast<short>(std::numeric_limits<char>::max()) + 1)};

template <auto Value>
constexpr auto CONSTEXPR_VALUE{Value};

#define CEXPR(expr) (CONSTEXPR_VALUE<expr>) // expr must be eval at compile-time

TEST(CommonStr, IsUpper) {
    using namespace oops::str;
    EXPECT_TRUE(CEXPR(IsUpper('A')));
    EXPECT_TRUE(CEXPR(IsUpper('Z')));

    EXPECT_FALSE(CEXPR(IsUpper('a')));
    EXPECT_FALSE(CEXPR(IsUpper('z')));
    EXPECT_FALSE(CEXPR(IsUpper('@')));
    EXPECT_FALSE(CEXPR(IsUpper('[')));

    EXPECT_FALSE(CEXPR(IsUpper('0')));
    EXPECT_FALSE(CEXPR(IsUpper(' ')));
    EXPECT_FALSE(CEXPR(IsUpper('\0')));
    EXPECT_FALSE(CEXPR(IsUpper('\n')));
}

TEST(CommonStr, IsUpperSameAsStd) {
    for (char c : CHAR_SET) {
        int c_int{static_cast<unsigned char>(c)};
        // TODO(resserops): 后续16进制通过formatter控制
        EXPECT_EQ(oops::str::IsUpper(c), std::isupper(c_int) != 0)
            << "Char: 0x" << std::hex << c_int << (std::isprint(c_int) ? std::string{" '"} + c + "'" : "");
    }
}

TEST(CommonStr, IsLower) {
    using namespace oops::str;
    EXPECT_TRUE(CEXPR(IsLower('a')));
    EXPECT_TRUE(CEXPR(IsLower('z')));

    EXPECT_FALSE(CEXPR(IsLower('A')));
    EXPECT_FALSE(CEXPR(IsLower('Z')));
    EXPECT_FALSE(CEXPR(IsLower('`')));
    EXPECT_FALSE(CEXPR(IsLower('{')));

    EXPECT_FALSE(CEXPR(IsLower('0')));
    EXPECT_FALSE(CEXPR(IsLower(' ')));
    EXPECT_FALSE(CEXPR(IsLower('\0')));
    EXPECT_FALSE(CEXPR(IsLower('\n')));
}

TEST(CommonStr, IsLowerSameAsStd) {
    for (char c : CHAR_SET) {
        int c_int{static_cast<unsigned char>(c)};
        // TODO(resserops): 后续16进制通过formatter控制
        EXPECT_EQ(oops::str::IsLower(c), std::islower(c_int) != 0)
            << "Char: 0x" << std::hex << c_int << (std::isprint(c_int) ? std::string{" '"} + c + "'" : "");
    }
}

TEST(CommonStr, IsAlpha) {
    using namespace oops::str;
    EXPECT_TRUE(CEXPR(IsAlpha('A')));
    EXPECT_TRUE(CEXPR(IsAlpha('Z')));
    EXPECT_TRUE(CEXPR(IsAlpha('a')));
    EXPECT_TRUE(CEXPR(IsAlpha('z')));

    EXPECT_FALSE(CEXPR(IsAlpha('@')));
    EXPECT_FALSE(CEXPR(IsAlpha('[')));
    EXPECT_FALSE(CEXPR(IsAlpha('`')));
    EXPECT_FALSE(CEXPR(IsAlpha('{')));

    EXPECT_FALSE(CEXPR(IsAlpha('0')));
    EXPECT_FALSE(CEXPR(IsAlpha(' ')));
    EXPECT_FALSE(CEXPR(IsAlpha('\0')));
    EXPECT_FALSE(CEXPR(IsAlpha('\n')));
}

TEST(CommonStr, IsAlphaSameAsStd) {
    for (char c : CHAR_SET) {
        int c_int{static_cast<unsigned char>(c)};
        // TODO(resserops): 后续16进制通过formatter控制
        EXPECT_EQ(oops::str::IsAlpha(c), std::isalpha(c_int) != 0)
            << "Char: 0x" << std::hex << c_int << (std::isprint(c_int) ? std::string{" '"} + c + "'" : "");
    }
}

TEST(CommonStr, IsDigit) {
    using namespace oops::str;
    EXPECT_TRUE(CEXPR(IsDigit('0')));
    EXPECT_TRUE(CEXPR(IsDigit('9')));

    EXPECT_FALSE(CEXPR(IsDigit('/')));
    EXPECT_FALSE(CEXPR(IsDigit(':')));

    EXPECT_FALSE(CEXPR(IsDigit('A')));
    EXPECT_FALSE(CEXPR(IsDigit('a')));
    EXPECT_FALSE(CEXPR(IsDigit(' ')));
    EXPECT_FALSE(CEXPR(IsDigit('\0')));
    EXPECT_FALSE(CEXPR(IsDigit('\n')));
}

TEST(CommonStr, IsDigitSameAsStd) {
    for (char c : CHAR_SET) {
        int c_int{static_cast<unsigned char>(c)};
        // TODO(resserops): 后续16进制通过formatter控制
        EXPECT_EQ(oops::str::IsDigit(c), std::isdigit(c_int) != 0)
            << "Char: 0x" << std::hex << c_int << (std::isprint(c_int) ? std::string{" '"} + c + "'" : "");
    }
}

TEST(CommonStr, IsAlnum) {
    using namespace oops::str;
    EXPECT_TRUE(CEXPR(IsAlnum('A')));
    EXPECT_TRUE(CEXPR(IsAlnum('Z')));
    EXPECT_TRUE(CEXPR(IsAlnum('a')));
    EXPECT_TRUE(CEXPR(IsAlnum('z')));
    EXPECT_TRUE(CEXPR(IsAlnum('0')));
    EXPECT_TRUE(CEXPR(IsAlnum('9')));

    EXPECT_FALSE(CEXPR(IsAlnum('@')));
    EXPECT_FALSE(CEXPR(IsAlnum('[')));
    EXPECT_FALSE(CEXPR(IsAlnum('`')));
    EXPECT_FALSE(CEXPR(IsAlnum('{')));
    EXPECT_FALSE(CEXPR(IsAlnum('/')));
    EXPECT_FALSE(CEXPR(IsAlnum(':')));

    EXPECT_FALSE(CEXPR(IsAlnum(' ')));
    EXPECT_FALSE(CEXPR(IsAlnum('\0')));
    EXPECT_FALSE(CEXPR(IsAlnum('\n')));
}

TEST(CommonStr, IsAlnumSameAsStd) {
    std::size_t count{0};
    for (char c : CHAR_SET) {
        int c_int{static_cast<unsigned char>(c)};
        // TODO(resserops): 后续16进制通过formatter控制
        EXPECT_EQ(oops::str::IsAlnum(c), std::isalnum(c_int) != 0)
            << "Char: 0x" << std::hex << c_int << (std::isprint(c_int) ? std::string{" '"} + c + "'" : "");
        ++count;
    }
    EXPECT_EQ(count, 1ull << __CHAR_BIT__);
}

TEST(CommonStr, ToLowerConstexpr) {
    using namespace oops::str;
    EXPECT_EQ(CEXPR(ToLower('A')), 'a');
    EXPECT_EQ(CEXPR(ToLower('Z')), 'z');

    EXPECT_EQ(CEXPR(ToLower('a')), 'a');
    EXPECT_EQ(CEXPR(ToLower('z')), 'z');
    EXPECT_EQ(CEXPR(ToLower('@')), '@');
    EXPECT_EQ(CEXPR(ToLower('[')), '[');

    EXPECT_EQ(CEXPR(ToLower('0')), '0');
    EXPECT_EQ(CEXPR(ToLower(' ')), ' ');
    EXPECT_EQ(CEXPR(ToLower('\0')), '\0');
    EXPECT_EQ(CEXPR(ToLower('\n')), '\n');
}

TEST(CommonStr, ToLowerSameAsStd) {
    for (char c : CHAR_SET) {
        int c_int{static_cast<unsigned char>(c)};
        // TODO(resserops): 后续16进制通过formatter控制
        EXPECT_EQ(oops::str::ToLower(c), static_cast<char>(std::tolower(c_int)))
            << "Char: 0x" << std::hex << c_int << (std::isprint(c_int) ? std::string{" '"} + c + "'" : "");
    }
}

TEST(CommonStr, ToUpper) {
    using namespace oops::str;
    EXPECT_EQ(CEXPR(ToLower('a')), 'a');
    EXPECT_EQ(CEXPR(ToLower('z')), 'z');

    EXPECT_EQ(CEXPR(ToLower('A')), 'a');
    EXPECT_EQ(CEXPR(ToLower('Z')), 'z');
    EXPECT_EQ(CEXPR(ToLower('`')), '`');
    EXPECT_EQ(CEXPR(ToLower('{')), '{');

    EXPECT_EQ(CEXPR(ToLower('0')), '0');
    EXPECT_EQ(CEXPR(ToLower(' ')), ' ');
    EXPECT_EQ(CEXPR(ToLower('\0')), '\0');
    EXPECT_EQ(CEXPR(ToLower('\n')), '\n');
}

TEST(CommonStr, ToUpperSameAsStd) {
    for (char c : CHAR_SET) {
        int c_int{static_cast<unsigned char>(c)};
        // TODO(resserops): 后续16进制通过formatter控制
        EXPECT_EQ(oops::str::ToUpper(c), static_cast<char>(std::toupper(c_int)))
            << "Char: 0x" << std::hex << c_int << (std::isprint(c_int) ? std::string{" '"} + c + "'" : "");
    }
}

TEST(CommonStr, StrRepeat) {
    using namespace oops;
    EXPECT_EQ(StrRepeat("abc", 0), "");
    EXPECT_EQ(StrRepeat("abc", 1), "abc");
    EXPECT_EQ(StrRepeat("abc", 3), "abcabcabc");
    EXPECT_EQ(StrRepeat("", 0), "");
    EXPECT_EQ(StrRepeat("", 1), "");
    EXPECT_EQ(StrRepeat("", 3), "");
}

TEST(CommonStr, StrSplitBack) {
    using namespace oops;
    EXPECT_EQ(StrSplitBack("abc/def/ghi", "/"), "ghi");
    EXPECT_EQ(StrSplitBack("abc/def/ghi/", "/"), "");
    EXPECT_EQ(StrSplitBack("/", "/"), "");
    EXPECT_EQ(StrSplitBack("abc", "/"), "abc");
    EXPECT_EQ(StrSplitBack("", "/"), "");
    EXPECT_EQ(StrSplitBack("abc", ""), "");
}

TEST(CommonStr, StrSplitToIter) {
    using namespace oops;
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
    using namespace oops;
    std::vector<std::string> res;
    StrSplitToIter("abc:defg:hi", ":", std::back_inserter(res));
    EXPECT_EQ(res, (std::vector<std::string>{"abc", "defg", "hi"}));
}

TEST(CommonStr, StartsWith) {
    using namespace oops::str;
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
    using namespace oops::str;
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
    using namespace oops::str;
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
