#pragma once
#include <cstddef>
#include <limits>
#include <numeric>
#include <ratio>
#include <type_traits>

namespace oops {
template <typename T>
struct IsRatio : std::false_type {};
template <std::intmax_t N, std::intmax_t D>
struct IsRatio<std::ratio<N, D>> : std::true_type {};
template <typename T>
constexpr bool IS_RATIO{IsRatio<T>::value};

// 存储容量单位定义
constexpr std::size_t BIT_PER_B{std::numeric_limits<unsigned char>::digits};
constexpr std::size_t B_PER_KIB{1024};
constexpr std::size_t B_PER_MIB{1024 * B_PER_KIB};
constexpr std::size_t B_PER_GIB{1024 * B_PER_MIB};
constexpr std::size_t B_PER_TIB{1024 * B_PER_GIB};

using Bit = std::ratio<1, BIT_PER_B>;
using Byte = std::ratio<1>;
using KiB = std::ratio<B_PER_KIB>;
using MiB = std::ratio<B_PER_MIB>;
using GiB = std::ratio<B_PER_GIB>;
using TiB = std::ratio<B_PER_TIB>;

template <typename R, typename P = Byte>
class Storage {
public:
    static_assert(std::is_arithmetic_v<R>);
    static_assert(IS_RATIO<P>);
    static_assert(P::num > 0);
    using Rep = R;
    using Period = P;

    constexpr Storage() = default;
    constexpr explicit Storage(const R &r) : count_(r) {}
    constexpr R Count() const { return count_; }

    // P2是否是P的整数倍
    template <typename P2>
    using IsHarmonic = std::bool_constant<std::ratio_divide<P2, P>::den == 1>;

    // 数值构造：要求类型可转换，目标类型R是浮点或源类型R2不是浮点，防止隐式截断
    template <
        typename R2,
        typename = std::enable_if_t<
            std::is_convertible_v<const R2 &, R> && (std::is_floating_point_v<R> || !std::is_floating_point_v<R2>)>>
    constexpr explicit Storage(const R2 &r) : count_(static_cast<R>(r)) {}

    // 复制构造：要求类型可转换，目标类型R是浮点或单位成整数倍且源类型R2不是浮点
    template <
        typename R2, typename P2,
        typename = std::enable_if_t<
            std::is_convertible_v<const R2 &, R> &&
            (std::is_floating_point_v<R> || (IsHarmonic<P2>::value && !std::is_floating_point_v<R2>))>>
    constexpr Storage(const Storage<R2, P2> &storage); // 在StorageCast后定义，以利用Cast功能

private:
    R count_{};
};

template <typename S, typename R, typename P>
constexpr S StorageCast(const Storage<R, P> &storage) {
    using Factor = std::ratio_divide<P, typename S::Period>;
    using CommonRep = std::common_type_t<typename S::Rep, R, std::intmax_t>;

    // 分流处理，优化运行时性能
    if constexpr (Factor::num == 1 && Factor::den == 1) {
        return S{static_cast<typename S::Rep>(storage.Count())};
    } else if constexpr (Factor::num != 1 && Factor::den == 1) {
        return S{static_cast<typename S::Rep>(
            static_cast<CommonRep>(storage.Count()) * static_cast<CommonRep>(Factor::num))};
    } else if constexpr (Factor::num == 1 && Factor::den != 1) {
        return S{static_cast<typename S::Rep>(
            static_cast<CommonRep>(storage.Count()) / static_cast<CommonRep>(Factor::den))};
    } else {
        return S{static_cast<typename S::Rep>(
            static_cast<CommonRep>(storage.Count()) * static_cast<CommonRep>(Factor::num) /
            static_cast<CommonRep>(Factor::den))};
    }
}

template <typename R, typename P>
template <typename R2, typename P2, typename>
constexpr Storage<R, P>::Storage(const Storage<R2, P2> &rhs) : count_{StorageCast<Storage>(rhs).Count()} {}

// 公共存储单位，S和S2向公共单位转换均无损
template <typename S, typename S2>
using CommonStorageT = Storage<
    std::common_type_t<typename S::Rep, typename S2::Rep>,
    std::ratio<std::gcd(S::Period::num, S2::Period::num), std::lcm(S::Period::den, S2::Period::den)>>;

// 不同Storage类型无损加减法
template <typename R, typename P, typename R2, typename P2>
constexpr CommonStorageT<Storage<R, P>, Storage<R2, P2>>
operator+(const Storage<R, P> &lhs, const Storage<R2, P2> &rhs) {
    using CommonStorage = CommonStorageT<Storage<R, P>, Storage<R2, P2>>;
    return CommonStorage{CommonStorage{lhs}.Count() + CommonStorage{rhs}.Count()};
}

template <typename R, typename P, typename R2, typename P2>
constexpr CommonStorageT<Storage<R, P>, Storage<R2, P2>>
operator-(const Storage<R, P> &lhs, const Storage<R2, P2> &rhs) {
    using CommonStorage = CommonStorageT<Storage<R, P>, Storage<R2, P2>>;
    return CommonStorage{CommonStorage{lhs}.Count() - CommonStorage{rhs}.Count()};
}

// 常用单位别名
using Bits = Storage<std::size_t, Bit>;
using Bytes = Storage<std::size_t, Byte>;
using KiBs = Storage<std::size_t, KiB>;
using MiBs = Storage<std::size_t, MiB>;
using GiBs = Storage<std::size_t, GiB>;
using TiBs = Storage<std::size_t, TiB>;
using KBs = Storage<std::size_t, std::kilo>;
using MBs = Storage<std::size_t, std::mega>;
using GBs = Storage<std::size_t, std::giga>;
using TBs = Storage<std::size_t, std::tera>;
} // namespace oops
