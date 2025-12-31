#include "oops/type_list.h"
#include <tuple>
using namespace oops::meta;

#define TEST_STATIC(suite, case) constexpr void TestStatic_##suite_##case ()

namespace {
class A {};
class B {};
class C {};
class D {};
class E {};
class F {};
} // namespace

// Identity
TEST_STATIC(MetaTypeList, Identity) {
    using Result = IdentityT<A>;
    using Expected = A;
    static_assert(std::is_same_v<Result, Expected>);
}

// Apply
TEST_STATIC(MetaTypeList, Apply) {
    using Result = ApplyT<std::tuple, TypeList<A>>;
    using Expected = std::tuple<A>;
    static_assert(std::is_same_v<Result, Expected>);
}

TEST_STATIC(MetaTypeList, ApplyZeroType) {
    using Result = ApplyT<std::tuple, TypeList<>>;
    using Expected = std::tuple<>;
    static_assert(std::is_same_v<Result, Expected>);
}

TEST_STATIC(MetaTypeList, ApplyMultiType) {
    using Result = ApplyT<std::tuple, TypeList<A, B, C>>;
    using Expected = std::tuple<A, B, C>;
    static_assert(std::is_same_v<Result, Expected>);
}

TEST_STATIC(MetaTypeList, ApplyDupType) {
    using Result = ApplyT<std::tuple, TypeList<A, A>>;
    using Expected = std::tuple<A, A>;
    static_assert(std::is_same_v<Result, Expected>);
}

TEST_STATIC(MetaTypeList, ApplyNestedType) {
    using Result = ApplyT<std::tuple, TypeList<TypeList<A>>>;
    using Expected = std::tuple<TypeList<A>>;
    static_assert(std::is_same_v<Result, Expected>);
}

TEST_STATIC(MetaTypeList, ApplyTuple) {
    using Result = ApplyT<TypeList, std::tuple<A>>;
    using Expected = TypeList<A>;
    static_assert(std::is_same_v<Result, Expected>);
}

// Transform
TEST_STATIC(MetaTypeList, Transform) {
    using Result = TransformT<std::tuple, TypeList<A>>;
    using Expected = TypeList<std::tuple<A>>;
    static_assert(std::is_same_v<Result, Expected>);
}

TEST_STATIC(MetaTypeList, TransformZeroType) {
    using Result = TransformT<std::tuple, TypeList<>>;
    using Expected = TypeList<>;
    static_assert(std::is_same_v<Result, Expected>);
}

TEST_STATIC(MetaTypeList, TransformMultiType) {
    using Result = TransformT<std::tuple, TypeList<A, B, C>>;
    using Expected = TypeList<std::tuple<A>, std::tuple<B>, std::tuple<C>>;
    static_assert(std::is_same_v<Result, Expected>);
}

TEST_STATIC(MetaTypeList, TransformDupType) {
    using Result = TransformT<std::tuple, TypeList<A, A>>;
    using Expected = TypeList<std::tuple<A>, std::tuple<A>>;
    static_assert(std::is_same_v<Result, Expected>);
}

TEST_STATIC(MetaTypeList, TransformNestType) {
    using Result = TransformT<std::tuple, TypeList<TypeList<A>>>;
    using Expected = TypeList<std::tuple<TypeList<A>>>;
    static_assert(std::is_same_v<Result, Expected>);
}

TEST_STATIC(MetaTypeList, TransformTuple) {
    using Result = TransformT<TypeList, std::tuple<A>>;
    using Expected = std::tuple<TypeList<A>>;
    static_assert(std::is_same_v<Result, Expected>);
}

// Concat
TEST_STATIC(MetaTypeList, Concat) {
    using Result = ConcatT<TypeList<A>, TypeList<B>>;
    using Expected = TypeList<A, B>;
    static_assert(std::is_same_v<Result, Expected>);
}

TEST_STATIC(MetaTypeList, ConcatZeroTypeList) {
    using Result = ConcatT<>;
    using Expected = TypeList<>;
    static_assert(std::is_same_v<Result, Expected>);
}

TEST_STATIC(MetaTypeList, ConcatOneTypeList) {
    using Result = ConcatT<TypeList<A>>;
    using Expected = TypeList<A>;
    static_assert(std::is_same_v<Result, Expected>);
}

TEST_STATIC(MetaTypeList, ConcatMultiTypeList) {
    using Result = ConcatT<TypeList<A>, TypeList<B>, TypeList<C>>;
    using Expected = TypeList<A, B, C>;
    static_assert(std::is_same_v<Result, Expected>);
}

TEST_STATIC(MetaTypeList, ConcatZeroType) {
    using Result = ConcatT<TypeList<>, TypeList<>>;
    using Expected = TypeList<>;
    static_assert(std::is_same_v<Result, Expected>);
}

TEST_STATIC(MetaTypeList, ConcatZeroType2) {
    using Result = ConcatT<TypeList<>, TypeList<A>>;
    using Expected = TypeList<A>;
    static_assert(std::is_same_v<Result, Expected>);
}

TEST_STATIC(MetaTypeList, ConcatZeroType3) {
    using Result = ConcatT<TypeList<A>, TypeList<>>;
    using Expected = TypeList<A>;
    static_assert(std::is_same_v<Result, Expected>);
}

TEST_STATIC(MetaTypeList, ConcatMultiType) {
    using Result = ConcatT<TypeList<A, B, C>, TypeList<D, E, F>>;
    using Expected = TypeList<A, B, C, D, E, F>;
    static_assert(std::is_same_v<Result, Expected>);
}

TEST_STATIC(MetaTypeList, ConcatDupType) {
    using Result = ConcatT<TypeList<A>, TypeList<A>>;
    using Expected = TypeList<A, A>;
    static_assert(std::is_same_v<Result, Expected>);
}

TEST_STATIC(MetaTypeList, ConcatNestType) {
    using Result = ConcatT<TypeList<TypeList<A>>, TypeList<TypeList<A>>>;
    using Expected = TypeList<TypeList<A>, TypeList<A>>;
    static_assert(std::is_same_v<Result, Expected>);
}

TEST_STATIC(MetaTypeList, ConcatComplex) {
    using Result = ConcatT<TypeList<A, A>, TypeList<>, TypeList<TypeList<>>, TypeList<B, C, D>, TypeList<E>>;
    using Expected = TypeList<A, A, TypeList<>, B, C, D, E>;
    static_assert(std::is_same_v<Result, Expected>);
}

// CartProd
TEST_STATIC(MetaTypeList, CartProd) {
    using Result = CartProdT<TypeList<A, B>, TypeList<C, D>>;
    using Expected = TypeList<TypeList<A, C>, TypeList<A, D>, TypeList<B, C>, TypeList<B, D>>;
    static_assert(std::is_same_v<Result, Expected>);
}

TEST_STATIC(MetaTypeList, CartProdZeroTypeList) {
    using Result = CartProdT<>;
    using Expected = TypeList<>;
    static_assert(std::is_same_v<Result, Expected>);
}

TEST_STATIC(MetaTypeList, CartProdOneTypeList) {
    using Result = CartProdT<TypeList<A, B>>;
    using Expected = TypeList<A, B>;
    static_assert(std::is_same_v<Result, Expected>);
}

TEST_STATIC(MetaTypeList, CartProdMultiTypeList) {
    using Result = CartProdT<TypeList<A, B>, TypeList<C, D>, TypeList<E, F>>;
    using Expected = TypeList<
        TypeList<A, C, E>, TypeList<A, C, F>, TypeList<A, D, E>, TypeList<A, D, F>, TypeList<B, C, E>,
        TypeList<B, C, F>, TypeList<B, D, E>, TypeList<B, D, F>>;
    static_assert(std::is_same_v<Result, Expected>);
}

TEST_STATIC(MetaTypeList, CartProdZeroType) {
    using Result = CartProdT<TypeList<>, TypeList<>>;
    using Expected = TypeList<>;
    static_assert(std::is_same_v<Result, Expected>);
}

TEST_STATIC(MetaTypeList, CartProdZeroType2) {
    using Result = CartProdT<TypeList<A, B>, TypeList<>>;
    using Expected = TypeList<>;
    static_assert(std::is_same_v<Result, Expected>);
}

TEST_STATIC(MetaTypeList, CartProdZeroType3) {
    using Result = CartProdT<TypeList<>, TypeList<A, B>>;
    using Expected = TypeList<>;
    static_assert(std::is_same_v<Result, Expected>);
}
