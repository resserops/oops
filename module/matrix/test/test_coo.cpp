#include <thread>

#include "oops/read.h"
#include "gtest/gtest.h"

using namespace oops;

TEST(Coo, ReadCoo) {
    std::ifstream iss(std::string(CASE_DIR) + "/crs.mtx");
    CooVar coo = ReadMatrixMarket(iss);
    std::visit([](const auto &coo) { std::cout << coo.M() << " " << coo.N() << " " << coo.Nnz() << std::endl; }, coo);
}

TEST(MatrixCoo, AnyCoo) {
    CooStore<double, int32_t> coo_store;
    coo_store.m = 3;
    coo_store.n = 3;
    coo_store.row_indices = {0, 0, 1, 1, 2, 2, 2};
    coo_store.col_indices = {0, 2, 1, 2, 0, 1, 2};
    coo_store.values = {28.4, 7.15, 10.9, 63.2, 7.15, 63.2, 45.7};

    Coo<double, int32_t> coo{coo_store};
    AnyCoo any_coo{coo};
    EXPECT_EQ(any_coo.M(), 3);
    EXPECT_EQ(any_coo.N(), 3);
    EXPECT_EQ(any_coo.Nnz(), 7);

    any_coo.ConvertInplace<float, int32_t>();
}
