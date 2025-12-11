#include <thread>

#include "oops/read.h"
#include "gtest/gtest.h"

using namespace oops;

constexpr const char *MTX{
    R"(%%MatrixMarket matrix coordinate real symmetric
3 3 4
1 1 3.2
2 2 5.4
3 1 2.0
3 3 1.1
)"};

TEST(Coo, ReadCoo) {
    std::istringstream iss(MTX);
    CooVar coo = ReadMatrixMarket(iss);
    std::visit([](const auto &coo) { std::cout << coo.M() << " " << coo.N() << " " << coo.Nnz() << std::endl; }, coo);
}
