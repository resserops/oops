#pragma once
#include "oops/coo.h"

using namespace oops::meta;
namespace oops {
AnyCoo ReadMatrixMarket(std::istream &is);
} // namespace oops
