#include "oops/view.h"
#include "gtest/gtest.h"

using namespace oops;

#define EXPECT_IL_EQ(lhs, ...) EXPECT_EQ(lhs, (decltype(lhs){__VA_ARGS__}))

TEST(CommonView, ReverseView) {
    std::vector<int> vec{1, 2, 3, 4, 5};
    std::vector<int> res;
    for (int i : Reverse(vec)) {
        res.push_back(i);
    }
    EXPECT_IL_EQ(res, 5, 4, 3, 2, 1);

    res.clear();
    for (const int &i : Reverse(vec)) {
        res.push_back(i);
    }
    EXPECT_IL_EQ(res, 5, 4, 3, 2, 1);

    res.clear();
    for (int &i : Reverse(vec)) {
        res.push_back(i);
        i += 2;
    }
    EXPECT_IL_EQ(res, 5, 4, 3, 2, 1);
    EXPECT_IL_EQ(vec, 3, 4, 5, 6, 7);

    auto r_view{Reverse(vec)};
    EXPECT_EQ(sizeof(decltype(r_view)), sizeof(void *));
}

TEST(CommonView, ReverseViewConst) {
    const std::vector<int> vec{1, 2, 3, 4, 5};
    std::vector<int> res;
    for (int i : Reverse(vec)) {
        res.push_back(i);
    }
    EXPECT_IL_EQ(res, 5, 4, 3, 2, 1);

    res.clear();
    for (const int &i : Reverse(vec)) {
        res.push_back(i);
    }
    EXPECT_IL_EQ(res, 5, 4, 3, 2, 1);

    auto r_view{Reverse(vec)};
    EXPECT_EQ(sizeof(decltype(r_view)), sizeof(void *));
}

TEST(CommonView, ReverseViewRvalue) {
    std::vector<int> res;
    for (int i : Reverse(std::vector<int>{1, 2, 3, 4, 5})) {
        res.push_back(i);
    }
    EXPECT_IL_EQ(res, 5, 4, 3, 2, 1);

    res.clear();
    for (const int &i : Reverse(std::vector<int>{1, 2, 3, 4, 5})) {
        res.push_back(i);
    }
    EXPECT_IL_EQ(res, 5, 4, 3, 2, 1);

    res.clear();
    for (int &i : Reverse(std::vector<int>{1, 2, 3, 4, 5})) {
        res.push_back(i);
    }
    EXPECT_IL_EQ(res, 5, 4, 3, 2, 1);

    auto r_view{Reverse(std::vector<int>{})};
    EXPECT_EQ(sizeof(decltype(r_view)), sizeof(std::vector<int>));
}

TEST(CommonView, ReverseViewNest) {
    std::vector<int> vec{1, 2, 3, 4, 5};
    std::vector<int> res;
    for (int &i : Reverse(Reverse(vec))) {
        res.push_back(i);
        i += 2;
    }
    EXPECT_IL_EQ(res, 1, 2, 3, 4, 5);
    EXPECT_IL_EQ(vec, 3, 4, 5, 6, 7);

    const std::vector<int> const_vec{1, 2, 3, 4, 5};
    res.clear();
    for (const int &i : Reverse(Reverse(const_vec))) {
        res.push_back(i);
    }
    EXPECT_IL_EQ(res, 1, 2, 3, 4, 5);
}

TEST(CommonView, ReverseViewNestConstIter) {
    std::vector<int> vec{1, 2, 3, 4, 5};
    std::vector<int> res;

    auto r_view{Reverse(vec)};
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
