#pragma once
#include <optional>
#include <vector>

#include "oops/matrix_type.h"

namespace oops {
// 数据存储类
template <typename Value, typename DimIndex, typename NnzIndex = DimIndex>
struct CsrStore {
    static_assert(std::is_integral_v<DimIndex>);
    static_assert(std::is_integral_v<NnzIndex>);

    using ValueType = Value;
    using DimIndexType = DimIndex;
    using NnzIndexType = NnzIndex;

    std::size_t n;
    std::vector<Value> values;
    std::vector<NnzIndex> row_ptr;
    std::vector<DimIndex> col_indices;
};

template <typename Value, typename DimIndex, typename NnzIndex = DimIndex>
class Csr {
public:
    template <typename OtherValue, typename OtherDimIndex, typename OtherNnzIndex>
    friend class Csr;

    using StoreType = CsrStore<Value, DimIndex, NnzIndex>;
    using ValueType = typename StoreType::ValueType;
    using DimIndexType = typename StoreType::DimIndexType;
    using NnzIndexType = typename StoreType::NnzIndexType;

    static constexpr MatrixFormat FORMAT{MatrixFormat::SPARSE_CSR};
    static constexpr MatrixNumeric VALUE_NUMERIC{MATRIX_NUMERIC_OF<Value>};
    static constexpr MatrixNumeric DIM_INDEX_NUMERIC{MATRIX_NUMERIC_OF<DimIndex>};
    static constexpr MatrixNumeric NNZ_INDEX_NUMERIC{MATRIX_NUMERIC_OF<NnzIndex>};

    Csr() = default;
    // "pass-by-value + move" idiom
    Csr(StoreType store) : store_{std::move(store)} {}
    Csr(StoreType store, MatrixSymmetric symmetric) : store_{std::move(store)}, symmetric_{symmetric} {}

    Csr(const Csr &) = default;
    Csr(Csr &&) noexcept = default;

    template <typename OtherValue, typename OtherDimIndex, typename OtherNnzIndex>
    Csr(const Csr<OtherValue, OtherDimIndex, OtherNnzIndex> &rhs)
        : store_{ConvertStore(rhs.store_)}, symmetric_{rhs.symmetric_} {}
    template <typename OtherValue, typename OtherDimIndex, typename OtherNnzIndex>
    Csr(Csr<OtherValue, OtherDimIndex, OtherNnzIndex> &&rhs)
        : store_{ConvertStore(std::move(rhs.store_))}, symmetric_{rhs.symmetric_} {}

    Csr &operator=(const Csr &) = default;
    Csr &operator=(Csr &&) noexcept = default;

    template <typename OtherValue, typename OtherDimIndex, typename OtherNnzIndex>
    Csr &operator=(const Csr<OtherValue, OtherDimIndex, OtherNnzIndex> &rhs) {
        store_ = ConvertStore(rhs.store_);
        symmetric_ = rhs.symmetric_;
        return *this;
    }
    template <typename OtherValue, typename OtherDimIndex, typename OtherNnzIndex>
    Csr &operator=(Csr<OtherValue, OtherDimIndex, OtherNnzIndex> &&rhs) {
        store_ = ConvertStore(std::move(rhs.store_));
        symmetric_ = rhs.symmetric_;
        return *this;
    }

    static constexpr MatrixFormat GetFormat() { return FORMAT; }
    static constexpr MatrixNumeric GetValueNumeric() { return VALUE_NUMERIC; }
    static constexpr MatrixNumeric GetDimIndexNumeric() { return DIM_INDEX_NUMERIC; }
    static constexpr MatrixNumeric GetNnzIndexNumeric() { return NNZ_INDEX_NUMERIC; }
    MatrixSymmetric GetSymmetric() const { return symmetric_; }

    std::size_t M() const { return store_.row_ptr.size() - 1; }
    std::size_t N() const { return store_.n; }
    std::size_t StoredNnz() const { return store_.col_indices.size(); }

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
    const std::vector<DimIndex> &GetRowPtr() const { return store_.row_ptr; }
    const std::vector<DimIndex> &GetColIndices() const { return store_.col_indices; }
    const StoreType &GetStore() const { return store_; }

private:
    template <typename OtherValue, typename OtherDimIndex, typename OtherNnzIndex>
    static CsrStore<Value, DimIndex, NnzIndex>
    ConvertStore(const CsrStore<OtherValue, OtherDimIndex, OtherNnzIndex> &rhs) {
        return {
            rhs.n, ConvertVector<Value>(rhs.values), ConvertVector<NnzIndex>(rhs.row_ptr),
            ConvertVector<DimIndex>(rhs.col_indices)};
    }

    template <typename OtherValue, typename OtherDimIndex, typename OtherNnzIndex>
    static CsrStore<Value, DimIndex, NnzIndex> ConvertStore(CsrStore<OtherValue, OtherDimIndex, OtherNnzIndex> &&rhs) {
        return {
            rhs.n, ConvertVector<Value>(std::move(rhs.values)), ConvertVector<NnzIndex>(std::move(rhs.row_ptr)),
            ConvertVector<DimIndex>(std::move(rhs.col_indices))};
    }

    DimIndex ComputeDiagNnz() const {
        DimIndex count{0};
        for (std::size_t r{0}; r < M(); ++r) {
            for (auto i{store_.row_ptr[r]}; i < store_.row_ptr[r + 1]; ++i) {
                if (r == static_cast<std::size_t>(store_.col_indices[i])) {
                    ++count;
                }
            }
        }
        return count;
    }

    StoreType store_;
    MatrixSymmetric symmetric_{MatrixSymmetric::GENERAL};
    mutable std::optional<DimIndex> diag_nnz_;
};
} // namespace oops
