#include "oops/csr.h"
#include "gtest/gtest.h"

using namespace oops;

// [2.3 7.8  .   .  1.5]
// [ .   .   .   .   . ]
// [4.6  .  3.9  .  8.2]
// [ .   .   .   .   . ]
// [5.1  .   .   .  6.7]
TEST(Csr, GeneralSquare) {
    CsrStore<double, int32_t> store;
    store.n = 5;
    store.values = {2.3, 7.8, 1.5, 4.6, 3.9, 8.2, 5.1, 6.7};
    store.row_ptr = {0, 3, 3, 6, 6, 8};
    store.col_indices = {0, 1, 4, 0, 2, 4, 0, 4};

    auto check = [&store](const auto &csr) {
        EXPECT_EQ(csr.GetFormat(), MatrixFormat::SPARSE_CSR);
        EXPECT_EQ(csr.GetValueNumeric(), MatrixNumeric::REAL);
        EXPECT_EQ(csr.GetDimIndexNumeric(), MatrixNumeric::INTEGER);
        EXPECT_EQ(csr.GetNnzIndexNumeric(), MatrixNumeric::INTEGER);
        EXPECT_EQ(csr.GetSymmetric(), MatrixSymmetric::GENERAL);

        EXPECT_EQ(csr.M(), 5);
        EXPECT_EQ(csr.N(), 5);
        EXPECT_EQ(csr.StoredNnz(), 8);
        EXPECT_EQ(csr.DiagNnz(), 3);
        EXPECT_EQ(csr.Nnz(), 8);

        EXPECT_EQ(csr.GetValues(), store.values);
        EXPECT_EQ(csr.GetRowPtr(), store.row_ptr);
        EXPECT_EQ(csr.GetColIndices(), store.col_indices);
    };

    Csr<double, int32_t> csr{store};
    check(csr);
    Csr<double, int32_t> copy_constructed{csr};
    check(copy_constructed);
    Csr<double, int32_t> assigned;
    assigned = csr;
    check(assigned);
}
