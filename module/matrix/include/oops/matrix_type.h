#pragma once
#include <algorithm>
#include <complex>
#include <cstdint>
#include <iostream>
#include <type_traits>
#include <variant>
#include <vector>

#include <cxxabi.h>

#include "oops/type_list.h"
namespace oops {
enum class MatrixFormat : uint8_t { SPARSE_COO, SPARSE_CSR, SPARSE_CSC, DENSE_ROW_MAJOR, DENSE_COL_MAJOR };
enum class MatrixNumeric : uint8_t { REAL, COMPLEX, INTEGER, PATTERN, OTHER };
enum class MatrixSymmetric : uint8_t {
    GENERAL,
    SYMMETRIC_LOWWER,
    SYMMETRIC_UPPER,
    HERMITIAN_LOWER,
    HERMITIAN_UPPER,
    SKEW_LOWER,
    SKEW_UPPER
};

template <auto V>
using IntegralConstant = std::integral_constant<decltype(V), V>;

// MatrixNumericOf
template <typename T>
struct MatrixNumericOf : IntegralConstant<MatrixNumeric::OTHER> {};
template <typename T>
constexpr MatrixNumeric MATRIX_NUMERIC_OF{MatrixNumericOf<T>::value};

template <>
struct MatrixNumericOf<float> : IntegralConstant<MatrixNumeric::REAL> {};
template <>
struct MatrixNumericOf<double> : IntegralConstant<MatrixNumeric::REAL> {};
template <>
struct MatrixNumericOf<std::complex<float>> : IntegralConstant<MatrixNumeric::COMPLEX> {};
template <>
struct MatrixNumericOf<std::complex<double>> : IntegralConstant<MatrixNumeric::COMPLEX> {};
template <>
struct MatrixNumericOf<intmax_t> : IntegralConstant<MatrixNumeric::INTEGER> {};
template <>
struct MatrixNumericOf<std::monostate> : IntegralConstant<MatrixNumeric::PATTERN> {};

// IsComplex
template <typename T>
struct IsComplex : std::false_type {};
template <typename T>
constexpr bool IS_COMPLEX{IsComplex<T>::value};

template <typename T>
struct IsComplex<std::complex<T>> : std::true_type {};

template <typename T>
struct Debug;

// Index and value type list
using ValueTypeList =
    meta::TypeList<float, double, std::complex<float>, std::complex<double>, intmax_t, std::monostate>;
using IndexTypeList = meta::TypeList<int32_t, int64_t>;

using ValueTypeVar = meta::ApplyT<std::variant, meta::TransformT<meta::Identity, ValueTypeList>>;
using IndexTypeVar = meta::ApplyT<std::variant, meta::TransformT<meta::Identity, IndexTypeList>>;

// Type conversion
// 小类型传参优化，现代编译期会自动识别，引入额外复杂性，暂不使用
template <typename T>
struct IsSmall : std::bool_constant<sizeof(T) <= 2 * sizeof(void *)> {};
template <typename T>
constexpr bool IS_SMALL{IsSmall<T>::value};

template <typename T>
struct IsTriviallyCopyableSmall : std::bool_constant<std::is_trivially_copyable_v<T> && IS_SMALL<T>> {};
template <typename T>
constexpr bool IS_TRIVIALLY_COPYABLE_SMALL{IsTriviallyCopyableSmall<T>::value};

// 定义基础类型的转换，按需增加越界验证
template <typename Dst, typename Src>
Dst Convert(Src &&src) {
    if constexpr (std::is_convertible_v<Src, Dst>) {
        return static_cast<Dst>(std::forward<Src>(src));
    } else {
        throw std::runtime_error("invalid conversion");
    }
}

template <typename Dst, typename Src>
struct ConvertVectorImpl {
    static auto F(const std::vector<Src> &src) {
        std::vector<Dst> dst;
        dst.reserve(src.size());
        std::transform(
            src.begin(), src.end(), std::back_inserter(dst), [](const Src &src) { return Convert<Dst>(src); });
        return dst;
    }

    static auto F(std::vector<Src> &&src) {
        std::vector<Dst> dst;
        dst.reserve(src.size());
        std::transform(
            std::make_move_iterator(src.begin()), std::make_move_iterator(src.end()), std::back_inserter(dst),
            [](Src &&src) { return Convert<Dst>(std::move(src)); });
        return dst;
    }
};

template <typename Dst>
struct ConvertVectorImpl<Dst, Dst> {
    static auto F(std::vector<Dst> src) { return src; }
};

template <typename Dst, typename Src>
auto ConvertVector(const std::vector<Src> &src) {
    return ConvertVectorImpl<std::decay_t<Dst>, std::decay_t<Src>>::F(src);
}

template <typename Dst, typename Src>
auto ConvertVector(std::vector<Src> &&src) {
    return ConvertVectorImpl<std::decay_t<Dst>, std::decay_t<Src>>::F(std::move(src));
}
} // namespace oops
