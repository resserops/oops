#pragma once

#define OOPS_HAS_MEMBER(class, member)                                             \
    []() constexpr {                                                               \
        constexpr auto trait{::oops::impl::MemberTrait(                            \
            [](auto &&x) -> decltype((x.member), std::true_type{}) { return {}; }, \
            [](...) { return std::false_type{}; })};                               \
        return decltype(trait(std::declval<class>()))::value;                      \
    }()

#ifndef HAS_MEMBER
#define HAS_MEMBER(class, member) OOPS_HAS_MEMBER(class, member)
#endif

namespace oops {
namespace impl {
template <typename T, typename F>
struct MemberTrait : public T, public F {
    using T::operator();
    using F::operator();
    constexpr MemberTrait(T &&t, F &&f) : T(std::move(t)), F(std::move(f)) {}
};
} // namespace impl
} // namespace oops
