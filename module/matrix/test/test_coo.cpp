#include "oops/coo.h"
#include "gtest/gtest.h"

using namespace oops;

// [2.3 7.8  .   .  1.5]
// [ .   .   .   .   . ]
// [4.6  .  3.9  .  8.2]
// [ .   .   .   .   . ]
// [5.1  .   .   .  6.7]
TEST(Coo, GeneralSquare) {
    CooStore<double, int32_t> store;
    store.m = 5;
    store.n = 5;
    store.values = {2.3, 7.8, 1.5, 4.6, 3.9, 8.2, 5.1, 6.7};
    store.row_indices = {0, 0, 0, 2, 2, 2, 4, 4};
    store.col_indices = {0, 1, 4, 0, 2, 4, 0, 4};

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
    };

    Coo<double, int32_t> coo{store};
    check(coo);
    Coo<double, int32_t> copy_constructed{coo};
    check(copy_constructed);
    Coo<double, int32_t> assigned;
    assigned = coo;
    check(assigned);
}
