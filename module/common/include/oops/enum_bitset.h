#pragma once

#include <bitset>

namespace oops {
template <typename E>
constexpr auto ToUnderlying(E e) noexcept {
    static_assert(::std::is_enum_v<E>);
    return static_cast<::std::underlying_type_t<E>>(e);
}

template <typename E>
class EnumBitset : public ::std::bitset<ToUnderlying(E::COUNT)> {
public:
    static_assert(::std::is_enum_v<E>);
    using UnderlyingType = ::std::underlying_type_t<E>;
    static constexpr UnderlyingType COUNT{ToUnderlying(E::COUNT)};
    using Bitset = ::std::bitset<COUNT>;
    static constexpr Bitset ONE{1};

    using Bitset::bitset;
    using Bitset::operator|=;

    constexpr EnumBitset(const Bitset &rhs) : Bitset{rhs} {}
    EnumBitset(E e) : Bitset{ONE << static_cast<UnderlyingType>(e)} {}
    EnumBitset &operator|=(E e) {
        *(this) |= EnumBitset{e};
        return *this;
    }
};

template <typename E>
std::enable_if_t<std::is_enum<E>::value, EnumBitset<E>> operator|(const EnumBitset<E> &lhs, const E rhs) {
    return lhs | EnumBitset{rhs};
}

template <typename E>
std::enable_if_t<std::is_enum<E>::value, EnumBitset<E>> operator|(const E lhs, const E rhs) {
    return EnumBitset{lhs} | EnumBitset{rhs};
}
} // namespace oops
