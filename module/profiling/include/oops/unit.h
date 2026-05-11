#pragma once
#include <limits>
#include <numeric>
#include <ratio>

namespace oops {
template <typename T>
struct IsRatio : std::false_type {};
template <std::intmax_t N, std::intmax_t D>
struct IsRatio<std::ratio<N, D>> : std::true_type {};
template <typename T>
constexpr bool IS_RATIO{IsRatio<T>::value};

// TODO(resserops): 遗留问题，确认RatioDivideSafe含义和算法
template <typename R1, typename R2>
using RatioDivideSafe = std::ratio<
    (R1::num / std::gcd(R1::num, R2::num)) * (R2::den / std::gcd(R1::den, R2::den)),
    (R1::den / std::gcd(R1::den, R2::den)) * (R2::num / std::gcd(R1::num, R2::num))>;

// 存储容量单位定义
constexpr std::uintmax_t BIT_PER_B{std::numeric_limits<unsigned char>::digits};
constexpr std::uintmax_t B_PER_KIB{1024};
constexpr std::uintmax_t B_PER_MIB{1024 * B_PER_KIB};
constexpr std::uintmax_t B_PER_GIB{1024 * B_PER_MIB};
constexpr std::uintmax_t B_PER_TIB{1024 * B_PER_GIB};

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

    // 谐波关系检查：P2是否是P的整数倍
    template <typename P2>
    using IsHarmonic = std::bool_constant<RatioDivideSafe<P2, P>::den == 1>;

    // 从数值构造：类型可转换，且目标类型R是浮点或源类型R2不是浮点，防止隐式截断
    template <
        typename R2,
        typename = std::enable_if_t<
            std::is_convertible_v<const R2 &, R> && (std::is_floating_point_v<R> || !std::is_floating_point_v<R2>)>>
    constexpr explicit Storage(const R2 &r) : count_(static_cast<R>(r)) {}

    // 从其它Storage单位隐式构造：类型可转换，且不丢失精度（目标类型R是浮点或单位成整数倍且源类型R2不是浮点）
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
    using Factor = RatioDivideSafe<P, typename S::Period>;
    using CommonRep = std::common_type_t<typename S::Rep, R, std::intmax_t>;

    // 分流处理以优化性能并减少溢出风险
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

template <typename T = std::uintmax_t>
using Bits = Storage<T, Bit>;
template <typename T = std::uintmax_t>
using Bytes = Storage<T, Byte>;
template <typename T = std::uintmax_t>
using KiBs = Storage<T, KiB>;
template <typename T = std::uintmax_t>
using MiBs = Storage<T, MiB>;
template <typename T = std::uintmax_t>
using GiBs = Storage<T, GiB>;
template <typename T = std::uintmax_t>
using TiBs = Storage<T, TiB>;
template <typename T = std::uintmax_t>
using KBs = Storage<T, std::kilo>;
template <typename T = std::uintmax_t>
using MBs = Storage<T, std::mega>;
template <typename T = std::uintmax_t>
using GBs = Storage<T, std::giga>;
template <typename T = std::uintmax_t>
using TBs = Storage<T, std::tera>;
} // namespace oops
