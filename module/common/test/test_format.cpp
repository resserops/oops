#include "oops/format.h"
#include "oops/str.h"
#include <gtest/gtest.h>

using namespace oops;

TEST(CommonFormat, FFloat) {
    EXPECT_EQ(ToStr(FFloat{1.0 / 3}), "0.33");
    EXPECT_EQ(ToStr(FFloat{10.0 / 3}), "3.33");
    EXPECT_EQ(ToStr(FFloat{100.0 / 3}), "33.33");

    EXPECT_EQ(ToStr(FFloat{1.0 / 3}.SetPrecision(3)), "0.333");
    EXPECT_EQ(ToStr(FFloat{10.0 / 3}.SetPrecision(3)), "3.333");
    EXPECT_EQ(ToStr(FFloat{100.0 / 3}.SetPrecision(3)), "33.333");

    EXPECT_EQ(ToStr(FFloat{1.0 / 3}.Sci()), "3.33e-01");
    EXPECT_EQ(ToStr(FFloat{10.0 / 3}.Sci()), "3.33e+00");
    EXPECT_EQ(ToStr(FFloat{100.0 / 3}.Sci()), "3.33e+01");

    EXPECT_EQ(ToStr(FFloat{1.0 / 3}.Sci().SetPrecision(3)), "3.333e-01");
    EXPECT_EQ(ToStr(FFloat{10.0 / 3}.Sci().SetPrecision(3)), "3.333e+00");
    EXPECT_EQ(ToStr(FFloat{100.0 / 3}.Sci().SetPrecision(3)), "3.333e+01");
}

TEST(CommonFormat, FDouble) {
    EXPECT_EQ(ToStr(FDouble{1.0 / 3}), "0.33");
    EXPECT_EQ(ToStr(FDouble{10.0 / 3}), "3.33");
    EXPECT_EQ(ToStr(FDouble{100.0 / 3}), "33.33");

    EXPECT_EQ(ToStr(FDouble{1.0 / 3}.SetPrecision(3)), "0.333");
    EXPECT_EQ(ToStr(FDouble{10.0 / 3}.SetPrecision(3)), "3.333");
    EXPECT_EQ(ToStr(FDouble{100.0 / 3}.SetPrecision(3)), "33.333");

    EXPECT_EQ(ToStr(FDouble{1.0 / 3}.Sci()), "3.33e-01");
    EXPECT_EQ(ToStr(FDouble{10.0 / 3}.Sci()), "3.33e+00");
    EXPECT_EQ(ToStr(FDouble{100.0 / 3}.Sci()), "3.33e+01");

    EXPECT_EQ(ToStr(FDouble{1.0 / 3}.Sci().SetPrecision(3)), "3.333e-01");
    EXPECT_EQ(ToStr(FDouble{10.0 / 3}.Sci().SetPrecision(3)), "3.333e+00");
    EXPECT_EQ(ToStr(FDouble{100.0 / 3}.Sci().SetPrecision(3)), "3.333e+01");
}

TEST(CommonFormat, FTable) {
    FTable tfmtter;
    tfmtter.AppendRow(6.5, 3, "temp");
    tfmtter.AppendRow(7.4, 256, 'E');
    tfmtter.SetProp({FTable::RIGHT, 1, 1, 5});
    std::cout << tfmtter.SetDelim("|");
}
