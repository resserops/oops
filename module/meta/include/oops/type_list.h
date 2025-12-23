#pragma once
#include <type_traits>

namespace oops {
namespace meta {
// 类型列表
template <typename... Ts>
struct TypeList;

// Identity(T) = T
template <typename T>
struct Identity {
    using Type = T;
};
template <typename T>
using IdentityT = typename Identity<T>::Type;

// 支持任意类型序列
// Apply(F, TL<...>) = F<...>
template <template <typename...> typename F, typename TL>
struct Apply;
template <template <typename...> typename F, typename TL>
using ApplyT = typename Apply<F, TL>::Type;

template <template <typename...> typename F, template <typename...> typename L, typename... Ts>
struct Apply<F, L<Ts...>> : Identity<F<Ts...>> {};

// Transform(F, TL<A, B, ...>) = TL<F<A>>
template <template <typename...> typename F, typename TL>
struct Transform;
template <template <typename...> typename F, typename TL>
using TransformT = typename Transform<F, TL>::Type;

template <template <typename...> typename F, template <typename...> typename L, typename... Ts>
struct Transform<F, L<Ts...>> {
    using Type = L<F<Ts>...>;
};

// 支持TypeList
// Concat(<A, B>, <C>, <D, E>) = <A, B, C, D, E>
template <typename... TLs>
struct Concat;
template <typename... TLs>
using ConcatT = typename Concat<TLs...>::Type;

template <>
struct Concat<> : Identity<TypeList<>> {};
template <typename... Ts>
struct Concat<TypeList<Ts...>> : Identity<TypeList<Ts...>> {};
template <typename... Ts1, typename... Ts2, typename... R>
struct Concat<TypeList<Ts1...>, TypeList<Ts2...>, R...> : Identity<ConcatT<TypeList<Ts1..., Ts2...>, R...>> {};

namespace detail {
// 基于笛卡尔积结果扩展更多维度，用于推导多元笛卡尔积
// Result = CartProd(<A>, <B, C>) = <<A, B>, <A, C>>
// CartExtend(Result, <E, F>) = <<A, B, E>, <A, C, E>, <A, B, F>, <A, C, F>>
template <typename... TLs>
struct CartExtend;
template <typename... TLs>
using CartExtendT = typename CartExtend<TLs...>::Type;

template <typename... Ts>
struct CartExtend<TypeList<>, TypeList<Ts...>> : Identity<TypeList<>> {};

template <typename... Ts1, typename... R, typename... Ts2>
struct CartExtend<TypeList<TypeList<Ts1...>, R...>, TypeList<Ts2...>>
    : Identity<ConcatT<TypeList<TypeList<Ts1..., Ts2>...>, CartExtendT<TypeList<R...>, TypeList<Ts2...>>>> {};

template <typename Result, typename TL, typename... R>
struct CartExtend<Result, TL, R...> : Identity<CartExtendT<CartExtendT<Result, TL>, R...>> {};
} // namespace detail

// CartProd(<A, B>, <C, D>) = <<A, C>, <A, D>, <B, C>, <B, D>>
template <typename... TLs>
struct CartProd;
template <typename... TLs>
using CartProdT = typename CartProd<TLs...>::Type;

template <typename... Ts>
struct CartProd<TypeList<>, TypeList<Ts...>> : Identity<TypeList<>> {};

template <typename T, typename... R, typename... Ts>
struct CartProd<TypeList<T, R...>, TypeList<Ts...>>
    : Identity<ConcatT<TypeList<TypeList<T, Ts>...>, CartProdT<TypeList<R...>, TypeList<Ts...>>>> {};

template <typename TL1, typename TL2, typename... R>
struct CartProd<TL1, TL2, R...> : Identity<detail::CartExtendT<CartProdT<TL1, TL2>, R...>> {};
} // namespace meta
} // namespace oops
