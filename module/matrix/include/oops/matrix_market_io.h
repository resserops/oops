#pragma once
#include "oops/coo.h"

using namespace oops::meta;
namespace oops {
AnyCoo ReadMatrixMarket(std::istream &is);
void WriteMatrixMarket(std::ostream &os, const AnyCoo &coo);
} // namespace oops
