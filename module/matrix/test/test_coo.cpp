#include <thread>

#include "oops/read.h"
#include "gtest/gtest.h"

using namespace oops;

TEST(Coo, ReadCoo) {
    std::ifstream iss(std::string(CASE_DIR) + "/crs.mtx");
    CooVar coo = ReadMatrixMarket(iss);
    std::visit([](const auto &coo) { std::cout << coo.M() << " " << coo.N() << " " << coo.Nnz() << std::endl; }, coo);
}
