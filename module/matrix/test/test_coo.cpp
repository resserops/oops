#include "oops/coo.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

using namespace oops;
using namespace testing;

// [2.3 7.8  .   .  1.5]
// [ .   .   .   .   . ]
// [4.6  .  3.9  .  8.2]
// [ .   .   .   .   . ]
// [5.1  .   .   .  6.7]
const CooStore<double, int32_t> &GetGeneralSquareStore() {
    auto gen = [] {
        CooStore<double, int32_t> store;
        store.m = 5;
        store.n = 5;
        store.values = {2.3, 7.8, 1.5, 4.6, 3.9, 8.2, 5.1, 6.7};
        store.row_indices = {0, 0, 0, 2, 2, 2, 4, 4};
        store.col_indices = {0, 1, 4, 0, 2, 4, 0, 4};
        return store;
    };
    static auto store{gen()};
    return store;
}

TEST(Coo, GeneralSquare) {
    auto store{GetGeneralSquareStore()};
    auto check = [&store](const auto &coo) {
        EXPECT_EQ(coo.GetFormat(), MatrixFormat::SPARSE_COO);
        EXPECT_EQ(coo.GetValueNumeric(), MatrixNumeric::REAL);
        EXPECT_EQ(coo.GetDimIndexNumeric(), MatrixNumeric::INTEGER);
        EXPECT_EQ(coo.GetSymmetric(), MatrixSymmetric::GENERAL);

        EXPECT_EQ(coo.M(), 5);
        EXPECT_EQ(coo.N(), 5);
        EXPECT_EQ(coo.StoredNnz(), 8);
        EXPECT_EQ(coo.DiagNnz(), 3);
        EXPECT_EQ(coo.Nnz(), 8);

        EXPECT_EQ(coo.GetValues(), store.values);
        EXPECT_EQ(coo.GetRowIndices(), store.row_indices);
        EXPECT_EQ(coo.GetColIndices(), store.col_indices);

        EXPECT_EQ(coo.At(0, 0), 2.3); // TODO(liubiqian): At接口考虑对称压缩
        EXPECT_EQ(coo.At(0, 1), 7.8);
        EXPECT_EQ(coo.At(0, 4), 1.5);
        EXPECT_EQ(coo.At(2, 0), 4.6);
        EXPECT_EQ(coo.At(2, 2), 3.9);
        EXPECT_EQ(coo.At(2, 4), 8.2);
        EXPECT_EQ(coo.At(4, 0), 5.1);
        EXPECT_EQ(coo.At(4, 4), 6.7);
    };

    Coo<double, int32_t> coo{store};
    check(coo);
    Coo<double, int32_t> copy_constructed{coo};
    check(copy_constructed);
    Coo<double, int32_t> assigned;
    assigned = coo;
    check(assigned);
}

TEST(Coo, GeneralSquareIter) {
    Coo<double, int32_t> coo{GetGeneralSquareStore()};
    // iterator
    // range-based for loop
    for (auto [value, row_index, col_index] : coo) {
        static_assert(std::is_reference_v<decltype(value)>);
        static_assert(!std::is_const_v<std::remove_reference_t<decltype(value)>>);

        EXPECT_THAT(coo.GetValues(), Contains(value));
        EXPECT_THAT(coo.GetRowIndices(), Contains(row_index));
        EXPECT_THAT(coo.GetColIndices(), Contains(col_index));

        EXPECT_EQ(coo.At(row_index, col_index), value);
    }

    // stl algorithm
    auto it{std::find_if(coo.begin(), coo.end(), [](const auto &triplet) {
        return (triplet.row_index != 0) && (2 * triplet.row_index == triplet.col_index);
    })};
    static_assert(std::is_same_v<decltype(it), decltype(coo)::iterator>);
    EXPECT_EQ(it.GetValue(), 8.2);
}

TEST(Coo, GeneralSquareConstIter) {
    const Coo<double, int32_t> coo{GetGeneralSquareStore()};
    // const iterator
    // range-based for loop
    for (auto [value, row_index, col_index] : coo) {
        static_assert(std::is_reference_v<decltype(value)>);
        static_assert(std::is_const_v<std::remove_reference_t<decltype(value)>>);

        EXPECT_THAT(coo.GetValues(), Contains(value));
        EXPECT_THAT(coo.GetRowIndices(), Contains(row_index));
        EXPECT_THAT(coo.GetColIndices(), Contains(col_index));

        EXPECT_EQ(coo.At(row_index, col_index), value);
    }

    // stl algorithm
    auto const_it{std::find_if(coo.begin(), coo.end(), [](const auto &triplet) {
        return (triplet.row_index != 0) && (2 * triplet.row_index == triplet.col_index);
    })};
    static_assert(std::is_same_v<decltype(const_it), decltype(coo)::const_iterator>);
    EXPECT_EQ(const_it.GetValue(), 8.2);
}
