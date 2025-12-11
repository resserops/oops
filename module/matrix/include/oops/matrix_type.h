#pragma once
#include <complex>
#include <cstdint>
#include <type_traits>
#include <variant>

#include "oops/meta_type_list.h"
namespace oops {
enum class MatrixFormat : uint8_t { SPARSE_COO, SPARSE_CSR, SPARSE_CSC, DENSE_ROW_MAJOR, DENSE_COL_MAJOR, OTHER };
enum class MatrixNumeric : uint8_t { REAL, COMPLEX, INTEGER, PATTERN, OTHER };
enum class MatrixSymmetric : uint8_t { GENERAL, SYMMETRIC, HERMITIAN, SKEW, OTHER };

template <auto V>
using IntegralConstant = std::integral_constant<decltype(V), V>;

template <typename T>
struct MatrixNumericOf;
template <typename T>
constexpr MatrixNumeric MatrixNumericOfV{MatrixNumericOf<T>::value};

template <>
struct MatrixNumericOf<double> : IntegralConstant<MatrixNumeric::REAL> {};
template <>
struct MatrixNumericOf<std::complex<double>> : IntegralConstant<MatrixNumeric::COMPLEX> {};
template <>
struct MatrixNumericOf<intmax_t> : IntegralConstant<MatrixNumeric::INTEGER> {};
template <>
struct MatrixNumericOf<std::monostate> : IntegralConstant<MatrixNumeric::PATTERN> {};

using IndexType = meta::TypeList<int32_t, int64_t>;
using ValueType = meta::TypeList<double, std::complex<double>, intmax_t, std::monostate>;
} // namespace oops
