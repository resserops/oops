#include "gtest/gtest.h"
#include "oops/once.h"

using namespace oops;

TEST(CommonOnce, Once) {
    int count{0};
    for (int i{0}; i < 100; ++i) {
        ONCE() {
            ++count;
        }
    }
    EXPECT_EQ(count, 1);
}

TEST(CommonOnce, Twice) {
    int count{0};
    for (int i{0}; i < 100; ++i) { 
        TWICE() {
            ++count;
        }
    }
    EXPECT_EQ(count, 2);
}

TEST(CommonOnce, Only) {
    int count{0};
    for (int i{0}; i < 100; ++i) {
        ONLY(5 + 5) {
            ++count;
        }
    }
    EXPECT_EQ(count, 10);
}

TEST(CommonOnce, Every) {
    int count{0};
    for (int i{0}; i < 100; ++i) {
        EVERY(3) {
            EXPECT_EQ(i % 3, 0);
            ++count;
        }
    }
    EXPECT_EQ(count, 34);
}
