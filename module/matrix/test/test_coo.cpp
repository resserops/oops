#include <thread>

#include "oops/read.h"
#include "gtest/gtest.h"

using namespace oops;

TEST(Coo, ReadCoo) {
    std::ifstream ifs("/home/liubiqian/github/oops/module/matrix/test/G2_circuit.mtx");
    CooVar coo = ReadMatrixMarket(ifs);
    std::visit([](const auto &coo) { std::cout << coo.M() << " " << coo.N() << " " << coo.Nnz() << std::endl; }, coo);
}
