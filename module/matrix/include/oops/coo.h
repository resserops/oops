#pragma once
#include <cstddef>
#include <optional>
#include <vector>

#include "oops/matrix_type.h"
#include "oops/type_list.h"

namespace oops {
// 数据存储类
template <typename Value, typename DimIndex>
struct CooStore {
    static_assert(std::is_integral_v<DimIndex>);

    using ValueType = Value;
    using DimIndexType = DimIndex;

    std::size_t m;
    std::size_t n;
    std::vector<Value> values;
    std::vector<DimIndex> row_indices;
    std::vector<DimIndex> col_indices;
};

template <typename Value, typename DimIndex>
struct Triplet {
    Value value;
    DimIndex row_index;
    DimIndex col_index;
};

template <typename Value, typename DimIndex>
class CooIterator {
public:
    using StoreTypeBase = CooStore<std::remove_const_t<Value>, DimIndex>;
    using StoreType = std::conditional_t<std::is_const_v<Value>, const StoreTypeBase, StoreTypeBase>;
    using TripletProxy = Triplet<Value &, DimIndex>;

    using iterator_category = std::bidirectional_iterator_tag;
    using difference_type = std::ptrdiff_t;
    using value_type = TripletProxy;
    using pointer = value_type *;
    using reference = value_type &;

    CooIterator() = default;
    CooIterator(StoreType &store, std::size_t i) : store_{store}, i_{i} {}

    TripletProxy operator*() const { return {store_.values[i_], store_.row_indices[i_], store_.col_indices[i_]}; }

    Value &GetValue() const { return store_.values[i_]; }
    DimIndex GetRowIndex() const { return store_.row_indices[i_]; }
    DimIndex GetColIndex() const { return store_.col_indices[i_]; }

    CooIterator &operator++() {
        ++i_;
        return *this;
    }

    CooIterator operator++(int) {
        CooIterator iter{*this};
        ++(*this);
        return iter;
    }

    CooIterator &operator--() {
        --i_;
        return *this;
    }

    CooIterator operator--(int) {
        CooIterator iter{*this};
        --(*this);
        return iter;
    }

    bool operator==(const CooIterator &rhs) const { return ((&store_) == (&rhs.store_)) && (i_ == rhs.i_); }
    bool operator!=(const CooIterator &rhs) const { return !operator==(rhs); }

private:
    StoreType &store_;
    std::size_t i_{0};
};

template <typename Value, typename DimIndex>
class Coo {
public:
    template <typename OtherValue, typename OtherDimIndex>
    friend class Coo;

    using StoreType = CooStore<Value, DimIndex>;
    using ValueType = typename StoreType::ValueType;
    using DimIndexType = typename StoreType::DimIndexType;

    using iterator = CooIterator<Value, DimIndex>;
    using const_iterator = CooIterator<const Value, DimIndex>;

    static constexpr MatrixFormat FORMAT{MatrixFormat::SPARSE_COO};
    static constexpr MatrixNumeric VALUE_NUMERIC{MATRIX_NUMERIC_OF<Value>};
    static constexpr MatrixNumeric DIM_INDEX_NUMERIC{MATRIX_NUMERIC_OF<DimIndex>};

    Coo() = default;
    // "pass-by-value + move" idiom
    Coo(StoreType store) : store_{std::move(store)} {}
    Coo(StoreType store, MatrixSymmetric symmetirc) : store_{std::move(store)}, symmetric_{symmetirc} {}

    Coo(const Coo &) = default;
    Coo(Coo &&) noexcept = default;

    template <typename OtherValue, typename OtherDimIndex>
    Coo(const Coo<OtherValue, OtherDimIndex> &rhs) : store_{ConvertStore(rhs.store_)}, symmetric_{rhs.symmetric_} {}
    template <typename OtherValue, typename OtherDimIndex>
    Coo(Coo<OtherValue, OtherDimIndex> &&rhs)
        : store_{ConvertStore(std::move(rhs.store_))}, symmetric_{rhs.symmetric_} {}

    Coo &operator=(const Coo &) = default;
    Coo &operator=(Coo &&) noexcept = default;

    template <typename OtherValue, typename OtherDimIndex>
    Coo &operator=(const Coo<OtherValue, OtherDimIndex> &rhs) {
        store_ = ConvertStore(rhs.store_);
        symmetric_ = rhs.symmetric_;
        return *this;
    }
    template <typename OtherValue, typename OtherDimIndex>
    Coo &operator=(Coo<OtherValue, OtherDimIndex> &&rhs) {
        store_ = ConvertStore(std::move(rhs.store_));
        symmetric_ = rhs.symmetric_;
        return *this;
    }

    iterator begin() { return iterator{store_, 0}; }
    iterator end() { return iterator{store_, StoredNnz()}; }
    const_iterator begin() const { return const_iterator{store_, 0}; }
    const_iterator end() const { return const_iterator{store_, StoredNnz()}; }

    const Value &At(DimIndex row_index, DimIndex col_index) const {
        for (auto it{begin()}; it != end(); ++it) {
            if (it.GetRowIndex() == row_index && it.GetColIndex() == col_index) {
                return it.GetValue();
            }
        }
        std::ostringstream oss;
        oss << "not found nnz at (" << row_index << ", " << col_index << ")";
        throw std::out_of_range(oss.str());
    }

    Value &At(DimIndex row_index, DimIndex col_index) {
        for (auto it{begin()}; it != end(); ++it) {
            if (it.GetRowIndex() == row_index && it.GetColIndex() == col_index) {
                return it.GetValue();
            }
        }
        std::ostringstream oss;
        oss << "not found nnz at (" << row_index << ", " << col_index << ")";
        throw std::out_of_range(oss.str());
    }

    static constexpr MatrixFormat GetFormat() { return FORMAT; }
    static constexpr MatrixNumeric GetValueNumeric() { return VALUE_NUMERIC; }
    static constexpr MatrixNumeric GetDimIndexNumeric() { return DIM_INDEX_NUMERIC; }
    MatrixSymmetric GetSymmetric() const { return symmetric_; }

    std::size_t M() const { return store_.m; }
    std::size_t N() const { return store_.n; }
    // 不应使用values，特殊情况partten矩阵不存储values
    std::size_t StoredNnz() const { return store_.row_indices.size(); }

    std::size_t DiagNnz() const {
        if (!diag_nnz_) {
            diag_nnz_ = ComputeDiagNnz();
        }
        return *diag_nnz_;
    }

    std::size_t Nnz() const {
        if (symmetric_ == MatrixSymmetric::GENERAL) {
            return StoredNnz();
        }
        return 2 * StoredNnz() - DiagNnz();
    }

    const std::vector<Value> &GetValues() const { return store_.values; }
    const std::vector<DimIndex> &GetRowIndices() const { return store_.row_indices; }
    const std::vector<DimIndex> &GetColIndices() const { return store_.col_indices; }
    const StoreType &GetStore() const { return store_; }

    // 提取内部Store所有权
    StoreType &ExtractStore() { return std::move(store_); }

private:
    template <typename OtherValue, typename OtherDimIndex>
    static CooStore<Value, DimIndex> ConvertStore(const CooStore<OtherValue, OtherDimIndex> &rhs) {
        return {
            rhs.m, rhs.n, ConvertVector<Value>(rhs.values), ConvertVector<DimIndex>(rhs.row_indices),
            ConvertVector<DimIndex>(rhs.col_indices)};
    }

    template <typename OtherValue, typename OtherDimIndex>
    static CooStore<Value, DimIndex> ConvertStore(CooStore<OtherValue, OtherDimIndex> &&rhs) {
        return {
            rhs.m, rhs.n, ConvertVector<Value>(std::move(rhs.values)),
            ConvertVector<DimIndex>(std::move(rhs.row_indices)), ConvertVector<DimIndex>(std::move(rhs.col_indices))};
    }

    DimIndex ComputeDiagNnz() const {
        DimIndex count{0};
        for (std::size_t i{0}; i < StoredNnz(); ++i) {
            if (store_.row_indices[i] == store_.col_indices[i]) {
                ++count;
            }
        }
        return count;
    }

    StoreType store_;
    MatrixSymmetric symmetric_{MatrixSymmetric::GENERAL};
    mutable std::optional<DimIndex> diag_nnz_;
};

template <typename TL>
using ApplyToCoo = meta::ApplyT<Coo, TL>;
using CooVar = meta::ApplyT<std::variant, meta::TransformT<ApplyToCoo, meta::CartProdT<ValueTypeList, IndexTypeList>>>;

#define OOPS_DEFINE_VISITOR(name)                                 \
    auto name() const {                                           \
        return Visit([](const auto &var) { return var.name(); }); \
    }

class AnyCoo {
public:
    template <typename Value, typename DimIndex>
    AnyCoo(Coo<Value, DimIndex> coo) : coo_var_{std::move(coo)} {}

    template <typename F>
    auto Visit(F &&f) {
        return std::visit(std::forward<F>(f), coo_var_);
    }

    template <typename F>
    auto Visit(F &&f) const {
        return std::visit(std::forward<F>(f), coo_var_);
    }

    OOPS_DEFINE_VISITOR(GetFormat);
    OOPS_DEFINE_VISITOR(GetValueNumeric);
    OOPS_DEFINE_VISITOR(GetDimIndexNumeric);
    OOPS_DEFINE_VISITOR(GetSymmetric);
    OOPS_DEFINE_VISITOR(M);
    OOPS_DEFINE_VISITOR(N);
    OOPS_DEFINE_VISITOR(StoredNnz);
    OOPS_DEFINE_VISITOR(DiagNnz);
    OOPS_DEFINE_VISITOR(Nnz);

    template <typename Value, typename DimIndex>
    const Coo<Value, DimIndex> &Get() const {
        return std::get<Coo<Value, DimIndex>>(coo_var_);
    }

    template <typename Value, typename DimIndex>
    Coo<Value, DimIndex> Convert() const {
        return Visit([](const auto &var) { return Coo<Value, DimIndex>{var}; });
    }

    template <typename Value, typename DimIndex>
    void ConvertInplace() {
        coo_var_ = Convert<Value, DimIndex>();
    }

private:
    CooVar coo_var_;
};

#undef OOPS_DEFINE_VISITOR
} // namespace oops
