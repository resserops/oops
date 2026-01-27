#pragma once
#include <cstddef>
#include <vector>

#include "oops/matrix_type.h"
#include "oops/type_list.h"

namespace oops {
// 数据存储类
template <typename Value, typename DimIndex>
struct CooStore {
    using ValueType = Value;
    using DimIndexType = DimIndex;
    std::size_t m;
    std::size_t n;
    std::vector<Value> values;
    std::vector<DimIndex> row_indices;
    std::vector<DimIndex> col_indices;
};

template <typename Value, typename DimIndex>
class Coo {
public:
    template <typename RhsValue, typename RhsDimIndex>
    friend class Coo;

    static_assert(std::is_integral_v<DimIndex>);
    using StoreType = CooStore<Value, DimIndex>;
    using ValueType = typename StoreType::ValueType;
    using DimIndexType = typename StoreType::DimIndexType;

    static constexpr MatrixFormat FORMAT{MatrixFormat::SPARSE_COO};
    static constexpr MatrixNumeric NUMERIC{MATRIX_NUMERIC_OF<Value>};

    Coo() = default;
    // "pass-by-value + move" idiom
    explicit Coo(StoreType store) : store_{std::move(store)} {}
    explicit Coo(StoreType store, MatrixSymmetric symmetirc) : store_{std::move(store)}, symmetric_{symmetirc} {}

    Coo(const Coo &) = default;
    Coo(Coo &&) noexcept = default;

    template <typename RhsValue, typename RhsDimIndex>
    Coo(const Coo<RhsValue, RhsDimIndex> &rhs) : store_{ConvertStore(rhs.store_)}, symmetric_{rhs.symmetric_} {}
    template <typename RhsValue, typename RhsDimIndex>
    Coo(Coo<RhsValue, RhsDimIndex> &&rhs) : store_{ConvertStore(std::move(rhs.store_))}, symmetric_{rhs.symmetric_} {}

    // 重构read后看是否能移除
    Coo &operator=(StoreType store) {
        store_ = std::move(store);
        return *this;
    }

    Coo &operator=(const Coo &) = default;
    Coo &operator=(Coo &&) noexcept = default;

    template <typename RhsValue, typename RhsDimIndex>
    Coo &operator=(const Coo<RhsValue, RhsDimIndex> &rhs) {
        store_ = ConvertStore(rhs.store_);
        symmetric_ = rhs.symmetric_;
        return *this;
    }
    template <typename RhsValue, typename RhsDimIndex>
    Coo &operator=(Coo<RhsValue, RhsDimIndex> &&rhs) {
        store_ = ConvertStore(std::move(rhs.store_));
        symmetric_ = rhs.symmetric_;
        return *this;
    }

    std::size_t M() const { return store_.m; }
    std::size_t N() const { return store_.n; }
    std::size_t Nnz() const { return store_.values.size(); }
    const std::vector<Value> &GetValues() const { return store_.values; }
    const std::vector<DimIndex> &GetRowIndices() const { return store_.row_indices; }
    const std::vector<DimIndex> &GetColIndices() const { return store_.col_indices; }

private:
    template <typename RhsValue, typename RhsDimIndex>
    static CooStore<Value, DimIndex> ConvertStore(const CooStore<RhsValue, RhsDimIndex> &rhs) {
        return {
            rhs.m, rhs.n, ConvertVector<Value>(rhs.values), ConvertVector<DimIndex>(rhs.row_indices),
            ConvertVector<DimIndex>(rhs.col_indices)};
    }

    template <typename RhsValue, typename RhsDimIndex>
    static CooStore<Value, DimIndex> ConvertStore(CooStore<RhsValue, RhsDimIndex> &&rhs) {
        return {
            rhs.m, rhs.n, ConvertVector<Value>(std::move(rhs.values)),
            ConvertVector<DimIndex>(std::move(rhs.row_indices)), ConvertVector<DimIndex>(std::move(rhs.col_indices))};
    }

    StoreType store_;
    MatrixSymmetric symmetric_{MatrixSymmetric::GENERAL};
};

template <typename TL>
using ApplyToCoo = meta::ApplyT<Coo, TL>;
using CooVar = meta::ApplyT<std::variant, meta::TransformT<ApplyToCoo, meta::CartProdT<ValueTypeList, IndexTypeList>>>;

class AnyCoo {
public:
    template <typename Value, typename DimIndex>
    explicit AnyCoo(Coo<Value, DimIndex> coo) : coo_var_{std::move(coo)} {}

    std::size_t M() const {
        auto f{[](const auto &var) { return var.M(); }};
        return std::visit(f, coo_var_);
    }
    std::size_t N() const {
        auto f{[](const auto &var) { return var.N(); }};
        return std::visit(f, coo_var_);
    }
    std::size_t Nnz() const {
        auto f{[](const auto &var) { return var.Nnz(); }};
        return std::visit(f, coo_var_);
    }

    template <typename Value, typename DimIndex>
    const Coo<Value, DimIndex> &Get() const {
        return std::get<Coo<Value, DimIndex>>(coo_var_);
    }

    template <typename Value, typename DimIndex>
    Coo<Value, DimIndex> Convert() const {
        auto f{[](const auto &var) { return Coo<Value, DimIndex>{var}; }};
        return std::visit(f, coo_var_);
    }

    template <typename Value, typename DimIndex>
    void ConvertInplace() {
        coo_var_ = Convert<Value, DimIndex>();
    }

private:
    CooVar coo_var_;
};
} // namespace oops
