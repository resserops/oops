#include "gtest/gtest.h"
#include "oops/range.h"

using namespace oops;

#define EXPECT_IL_EQ(lhs, ...) EXPECT_EQ(lhs, (decltype(lhs){__VA_ARGS__}))

TEST(CommonRange, ReverseRange) {
    std::vector<int> vec{1, 2, 3, 4, 5};
    std::vector<int> res;
    for (int i : ReverseRange(vec)) {
        res.push_back(i);
    }
    EXPECT_IL_EQ(res, 5, 4, 3, 2, 1);

    res.clear();
    for (const int &i : ReverseRange(vec)) {
        res.push_back(i);
    }
    EXPECT_IL_EQ(res, 5, 4, 3, 2, 1);

    res.clear();
    for (int &i : ReverseRange(vec)) {
        res.push_back(i);
        i += 2;
    }
    EXPECT_IL_EQ(res, 5, 4, 3, 2, 1);
    EXPECT_IL_EQ(vec, 3, 4, 5, 6, 7);

    auto r_view{ReverseRange(vec)};
    EXPECT_EQ(sizeof(decltype(r_view)), sizeof(void *));
}

TEST(CommonRange, ReverseRangeConst) {
    const std::vector<int> vec{1,2,3,4,5};
    std::vector<int> res;
    for (int i : ReverseRange(vec)) {
        res.push_back(i);
    }
    EXPECT_IL_EQ(res, 5, 4, 3, 2, 1);
    
    res.clear();
    for (const int &i : ReverseRange(vec)) {
        res.push_back(i);
    }
    EXPECT_IL_EQ(res, 5, 4, 3, 2, 1);

    auto r_view{ReverseRange(vec)};
    EXPECT_EQ(sizeof(decltype(r_view)), sizeof(void *));
}

TEST(CommonRange, ReverseRangeRvalue) {
    std::vector<int> res;
    for (int i : ReverseRange(std::vector<int>{1, 2, 3, 4, 5})) {
        res.push_back(i);
    }
    EXPECT_IL_EQ(res, 5, 4, 3, 2, 1);

    res.clear();
    for (const int &i : ReverseRange(std::vector<int>{1, 2, 3, 4, 5})) {
        res.push_back(i);
    }
    EXPECT_IL_EQ(res, 5, 4, 3, 2, 1);

    res.clear();
    for (int &i : ReverseRange(std::vector<int>{1, 2, 3, 4, 5})) {
        res.push_back(i);
    }
    EXPECT_IL_EQ(res, 5, 4, 3, 2, 1);

    auto r_view{ReverseRange(std::vector<int>{})};
    EXPECT_EQ(sizeof(decltype(r_view)), sizeof(std::vector<int>));
}

TEST(CommonRange, ReverseRangeNest) {
    std::vector<int> vec{1, 2, 3, 4, 5};
    std::vector<int> res;
    for (int &i : ReverseRange(ReverseRange(vec))) {
        res.push_back(i);
        i += 2;
    }
    EXPECT_IL_EQ(res, 1, 2, 3, 4, 5);
    EXPECT_IL_EQ(vec, 3, 4, 5, 6, 7);

    const std::vector<int> const_vec{1, 2, 3, 4, 5};
    res.clear();
    for (const int &i : ReverseRange(ReverseRange(const_vec))) {
        res.push_back(i);
    }
    EXPECT_IL_EQ(res, 1, 2, 3, 4, 5);
}

TEST(CommonRange, ReverseRangeNestConstIter) {
    std::vector<int> vec{1, 2, 3, 4, 5};
    std::vector<int> res;

    auto r_view{ReverseRange(vec)};
    res.clear();
    for (auto iter{r_view.cbegin()}; iter != r_view.cend(); ++iter) {
        res.push_back(*iter);
    }
    EXPECT_IL_EQ(res, 5, 4, 3, 2, 1);

    res.clear();
    for (auto iter{r_view.crbegin()}; iter != r_view.crend(); ++iter) {
        res.push_back(*iter);
    }
    EXPECT_IL_EQ(res, 1, 2, 3, 4, 5);
}
