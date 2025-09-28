#include <type_traits>

#include "oops/range.h"
#include "oops/str.h"
#include "gtest/gtest.h"

static constexpr struct {
    class Iter {
    public:
        constexpr explicit Iter(short value) : value_{value} {}
        constexpr char operator*() const noexcept { return value_; }
        constexpr bool operator!=(Iter rhs) const noexcept { return value_ != rhs.value_; }
        constexpr Iter &operator++() noexcept {
            ++value_;
            return *this;
        }

    private:
        short value_;
    };

    constexpr Iter begin() const noexcept { return Iter{std::numeric_limits<char>::min()}; }
    constexpr Iter end() const noexcept { return Iter{static_cast<short>(std::numeric_limits<char>::max()) + 1}; }
} CHAR_SET;

template <auto Value>
constexpr auto CONSTEXPR_VALUE{Value};

#define CONSTEXPR(expr) (CONSTEXPR_VALUE<expr>) // TODO(liubiqian): 找一个合适的内存

TEST(CommonStr, IsUpper) {
    using namespace oops::str;
    EXPECT_TRUE(CONSTEXPR(IsUpper('A')));
    EXPECT_TRUE(CONSTEXPR(IsUpper('Z')));

    EXPECT_FALSE(CONSTEXPR(IsUpper('a')));
    EXPECT_FALSE(CONSTEXPR(IsUpper('z')));
    EXPECT_FALSE(CONSTEXPR(IsUpper('@')));
    EXPECT_FALSE(CONSTEXPR(IsUpper('[')));

    EXPECT_FALSE(CONSTEXPR(IsUpper('0')));
    EXPECT_FALSE(CONSTEXPR(IsUpper(' ')));
    EXPECT_FALSE(CONSTEXPR(IsUpper('\0')));
    EXPECT_FALSE(CONSTEXPR(IsUpper('\n')));
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
    EXPECT_TRUE(CONSTEXPR(IsLower('a')));
    EXPECT_TRUE(CONSTEXPR(IsLower('z')));

    EXPECT_FALSE(CONSTEXPR(IsLower('A')));
    EXPECT_FALSE(CONSTEXPR(IsLower('Z')));
    EXPECT_FALSE(CONSTEXPR(IsLower('`')));
    EXPECT_FALSE(CONSTEXPR(IsLower('{')));

    EXPECT_FALSE(CONSTEXPR(IsLower('0')));
    EXPECT_FALSE(CONSTEXPR(IsLower(' ')));
    EXPECT_FALSE(CONSTEXPR(IsLower('\0')));
    EXPECT_FALSE(CONSTEXPR(IsLower('\n')));
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
    EXPECT_TRUE(CONSTEXPR(IsAlpha('A')));
    EXPECT_TRUE(CONSTEXPR(IsAlpha('Z')));
    EXPECT_TRUE(CONSTEXPR(IsAlpha('a')));
    EXPECT_TRUE(CONSTEXPR(IsAlpha('z')));

    EXPECT_FALSE(CONSTEXPR(IsAlpha('@')));
    EXPECT_FALSE(CONSTEXPR(IsAlpha('[')));
    EXPECT_FALSE(CONSTEXPR(IsAlpha('`')));
    EXPECT_FALSE(CONSTEXPR(IsAlpha('{')));

    EXPECT_FALSE(CONSTEXPR(IsAlpha('0')));
    EXPECT_FALSE(CONSTEXPR(IsAlpha(' ')));
    EXPECT_FALSE(CONSTEXPR(IsAlpha('\0')));
    EXPECT_FALSE(CONSTEXPR(IsAlpha('\n')));
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
    EXPECT_TRUE(CONSTEXPR(IsDigit('0')));
    EXPECT_TRUE(CONSTEXPR(IsDigit('9')));

    EXPECT_FALSE(CONSTEXPR(IsDigit('/')));
    EXPECT_FALSE(CONSTEXPR(IsDigit(':')));

    EXPECT_FALSE(CONSTEXPR(IsDigit('A')));
    EXPECT_FALSE(CONSTEXPR(IsDigit('a')));
    EXPECT_FALSE(CONSTEXPR(IsDigit(' ')));
    EXPECT_FALSE(CONSTEXPR(IsDigit('\0')));
    EXPECT_FALSE(CONSTEXPR(IsDigit('\n')));
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
    EXPECT_TRUE(CONSTEXPR(IsAlnum('A')));
    EXPECT_TRUE(CONSTEXPR(IsAlnum('Z')));
    EXPECT_TRUE(CONSTEXPR(IsAlnum('a')));
    EXPECT_TRUE(CONSTEXPR(IsAlnum('z')));
    EXPECT_TRUE(CONSTEXPR(IsAlnum('0')));
    EXPECT_TRUE(CONSTEXPR(IsAlnum('9')));

    EXPECT_FALSE(CONSTEXPR(IsAlnum('@')));
    EXPECT_FALSE(CONSTEXPR(IsAlnum('[')));
    EXPECT_FALSE(CONSTEXPR(IsAlnum('`')));
    EXPECT_FALSE(CONSTEXPR(IsAlnum('{')));
    EXPECT_FALSE(CONSTEXPR(IsAlnum('/')));
    EXPECT_FALSE(CONSTEXPR(IsAlnum(':')));

    EXPECT_FALSE(CONSTEXPR(IsAlnum(' ')));
    EXPECT_FALSE(CONSTEXPR(IsAlnum('\0')));
    EXPECT_FALSE(CONSTEXPR(IsAlnum('\n')));
}

TEST(CommonStr, IsAlnumSameAsStd) {
    for (char c : CHAR_SET) {
        int c_int{static_cast<unsigned char>(c)};
        // TODO(resserops): 后续16进制通过formatter控制
        EXPECT_EQ(oops::str::IsAlnum(c), std::isalnum(c_int) != 0)
            << "Char: 0x" << std::hex << c_int << (std::isprint(c_int) ? std::string{" '"} + c + "'" : "");
    }
}

TEST(CommonStr, ToLowerConstexpr) {
    using namespace oops::str;
    EXPECT_EQ(CONSTEXPR(ToLower('A')), 'a');
    EXPECT_EQ(CONSTEXPR(ToLower('Z')), 'z');

    EXPECT_EQ(CONSTEXPR(ToLower('a')), 'a');
    EXPECT_EQ(CONSTEXPR(ToLower('z')), 'z');
    EXPECT_EQ(CONSTEXPR(ToLower('@')), '@');
    EXPECT_EQ(CONSTEXPR(ToLower('[')), '[');

    EXPECT_EQ(CONSTEXPR(ToLower('0')), '0');
    EXPECT_EQ(CONSTEXPR(ToLower(' ')), ' ');
    EXPECT_EQ(CONSTEXPR(ToLower('\0')), '\0');
    EXPECT_EQ(CONSTEXPR(ToLower('\n')), '\n');
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
    EXPECT_EQ(CONSTEXPR(ToLower('a')), 'a');
    EXPECT_EQ(CONSTEXPR(ToLower('z')), 'z');

    EXPECT_EQ(CONSTEXPR(ToLower('A')), 'a');
    EXPECT_EQ(CONSTEXPR(ToLower('Z')), 'z');
    EXPECT_EQ(CONSTEXPR(ToLower('`')), '`');
    EXPECT_EQ(CONSTEXPR(ToLower('{')), '{');

    EXPECT_EQ(CONSTEXPR(ToLower('0')), '0');
    EXPECT_EQ(CONSTEXPR(ToLower(' ')), ' ');
    EXPECT_EQ(CONSTEXPR(ToLower('\0')), '\0');
    EXPECT_EQ(CONSTEXPR(ToLower('\n')), '\n');
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

TEST(CommonStr, StrMul) {
    using namespace oops;
    using namespace std::literals;
    EXPECT_EQ("abc"s * 0, "");
    EXPECT_EQ("abc"s * 1, "abc");
    EXPECT_EQ("abc"s * 3, "abcabcabc");
    EXPECT_EQ(0 * "abc"s, "");
    EXPECT_EQ(1 * "abc"s, "abc");
    EXPECT_EQ(3 * "abc"s, "abcabcabc");
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
