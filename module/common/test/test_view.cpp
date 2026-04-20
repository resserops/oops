#include "oops/view.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

using namespace oops::view;
using namespace testing;

TEST(CommonView, Reverse) {
    std::vector<int> vec{1, 2, 3, 4, 5};
    std::vector<int> res;
    for (int i : Reverse(vec)) {
        res.push_back(i);
    }
    EXPECT_THAT(res, ElementsAre(5, 4, 3, 2, 1));

    res.clear();
    for (const int &i : Reverse(vec)) {
        res.push_back(i);
    }
    EXPECT_THAT(res, ElementsAre(5, 4, 3, 2, 1));

    res.clear();
    for (int &i : Reverse(vec)) {
        res.push_back(i);
        i += 2;
    }
    EXPECT_THAT(res, ElementsAre(5, 4, 3, 2, 1));
    EXPECT_THAT(vec, ElementsAre(3, 4, 5, 6, 7));

    auto r_view{Reverse(vec)};
    EXPECT_EQ(sizeof(decltype(r_view)), sizeof(void *));
}

TEST(CommonView, ReverseConst) {
    const std::vector<int> vec{1, 2, 3, 4, 5};
    std::vector<int> res;
    for (int i : Reverse(vec)) {
        res.push_back(i);
    }
    EXPECT_THAT(res, ElementsAre(5, 4, 3, 2, 1));

    res.clear();
    for (const int &i : Reverse(vec)) {
        res.push_back(i);
    }
    EXPECT_THAT(res, ElementsAre(5, 4, 3, 2, 1));

    auto r_view{Reverse(vec)};
    EXPECT_EQ(sizeof(decltype(r_view)), sizeof(void *));
}

TEST(CommonView, ReverseRvalue) {
    std::vector<int> res;
    for (int i : Reverse(std::vector<int>{1, 2, 3, 4, 5})) {
        res.push_back(i);
    }
    EXPECT_THAT(res, ElementsAre(5, 4, 3, 2, 1));

    res.clear();
    for (const int &i : Reverse(std::vector<int>{1, 2, 3, 4, 5})) {
        res.push_back(i);
    }
    EXPECT_THAT(res, ElementsAre(5, 4, 3, 2, 1));

    res.clear();
    for (int &i : Reverse(std::vector<int>{1, 2, 3, 4, 5})) {
        res.push_back(i);
    }
    EXPECT_THAT(res, ElementsAre(5, 4, 3, 2, 1));

    auto r_view{Reverse(std::vector<int>{})};
    EXPECT_EQ(sizeof(decltype(r_view)), sizeof(std::vector<int>));
}

TEST(CommonView, ReverseNest) {
    std::vector<int> vec{1, 2, 3, 4, 5};
    std::vector<int> res;
    for (int &i : Reverse(Reverse(vec))) {
        res.push_back(i);
        i += 2;
    }
    EXPECT_THAT(res, ElementsAre(1, 2, 3, 4, 5));
    EXPECT_THAT(vec, ElementsAre(3, 4, 5, 6, 7));

    const std::vector<int> const_vec{1, 2, 3, 4, 5};
    res.clear();
    for (const int &i : Reverse(Reverse(const_vec))) {
        res.push_back(i);
    }
    EXPECT_THAT(res, ElementsAre(1, 2, 3, 4, 5));
}

TEST(CommonView, ReverseNestConstIter) {
    std::vector<int> vec{1, 2, 3, 4, 5};
    std::vector<int> res;

    auto r_view{Reverse(vec)};
    res.clear();
    for (auto iter{r_view.cbegin()}; iter != r_view.cend(); ++iter) {
        res.push_back(*iter);
    }
    EXPECT_THAT(res, ElementsAre(5, 4, 3, 2, 1));

    res.clear();
    for (auto iter{r_view.crbegin()}; iter != r_view.crend(); ++iter) {
        res.push_back(*iter);
    }
    EXPECT_THAT(res, ElementsAre(1, 2, 3, 4, 5));
}

TEST(CommonView, Range) {
    std::vector<int> res;
    for (auto i : oops::view::Range(10)) {
        res.push_back(i);
    }
    EXPECT_THAT(res, ElementsAre(0, 1, 2, 3, 4, 5, 6, 7, 8, 9));

    res.clear();
    for (auto i : oops::view::Range(1, 11)) {
        res.push_back(i);
    }
    EXPECT_THAT(res, ElementsAre(1, 2, 3, 4, 5, 6, 7, 8, 9, 10));

    res.clear();
    for (auto i : oops::view::Range(0, 30, 5)) {
        res.push_back(i);
    }
    EXPECT_THAT(res, ElementsAre(0, 5, 10, 15, 20, 25));

    res.clear();
    for (auto i : oops::view::Range(0, 10, 3)) {
        res.push_back(i);
    }
    EXPECT_THAT(res, ElementsAre(0, 3, 6, 9));

    res.clear();
    for (auto i : oops::view::Range(0, -10, -1)) {
        res.push_back(i);
    }
    EXPECT_THAT(res, ElementsAre(0, -1, -2, -3, -4, -5, -6, -7, -8, -9));

    res.clear();
    for (auto i : oops::view::Range(0)) {
        res.push_back(i);
    }
    EXPECT_THAT(res, ElementsAre());

    res.clear();
    for (auto i : oops::view::Range(1, 0)) {
        res.push_back(i);
    }
    EXPECT_THAT(res, ElementsAre());
}

TEST(CommonView, IntegerSet) {
    int count{0};
    for ([[maybe_unused]] auto i : IntegerSet<char>) {
        ++count;
    }
    oops::view::Range((short)2, RANGE_OVERFLOW);
    oops::view::Range<short>(2, 21);
    oops::view::Range<int>(21);
    oops::view::Range<int>(RANGE_OVERFLOW);
    oops::view::Range(10);
    // oops::view::Range(RANGE_OVERFLOW);
    EXPECT_EQ(count, 256);
}
