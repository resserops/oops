#pragma once
#include <cstddef>
#include <vector>

#include "oops/matrix_type.h"

namespace oops {
// 数据存储类
template <typename Value, typename DimIndex>
struct CooStore {
    std::size_t m;
    std::size_t n;
    std::vector<Value> values;
    std::vector<DimIndex> row_idx;
    std::vector<DimIndex> col_idx;
};

template <typename Value, typename DimIndex>
class Coo {
public:
    using ValueType = Value;
    using DimIndexType = DimIndex;
    using StoreType = CooStore<Value, DimIndex>;

    static constexpr MatrixFormat FORMAT{MatrixFormat::SPARSE_COO};
    static constexpr MatrixNumeric NUMERIC{MatrixNumericOfV<Value>};

    Coo() = default;
    Coo(StoreType store) : store_{std::move(store)} {}
    Coo &operator=(StoreType store) {
        store_ = std::move(store);
        return *this;
    }

    std::size_t M() const { return store_.m; }
    std::size_t N() const { return store_.n; }
    std::size_t Nnz() const { return store_.values.size(); }
    const std::vector<DimIndex> &GetRowIdx() const { return store_.row_idx; }
    const std::vector<DimIndex> &GetColIdx() const { return store_.col_idx; }
    const std::vector<Value> &GetValues() const { return store_.values; }

private:
    StoreType store_;
};

template <typename Value, typename DimIndex>
class CooMatrix : public Coo<Value, DimIndex> {
public:
private:
    MatrixSymmetric symmetric_;
};
} // namespace oops
