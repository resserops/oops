#include "oops/matrix_market_io.h"
#include "oops/enum_bitset.h" // for ToUnderlying

#include "oops/str.h"

namespace oops {
template <typename Value, typename DimIndex>
static auto ReadMatrixMarketStore(std::istream &is, std::size_t m, std::size_t n, std::size_t stored_nnz) {
    CooStore<Value, DimIndex> store;
    store.m = m;
    store.n = n;

    DimIndex row_index, col_index;
    store.row_indices.reserve(stored_nnz);
    store.col_indices.reserve(stored_nnz);
    if constexpr (std::is_same_v<Value, std::monostate>) {
        for (std::size_t i{0}; i < stored_nnz; ++i) {
            if (!(is >> row_index >> col_index)) {
                throw std::runtime_error("failed to read pattern entry");
            }
            store.row_indices.push_back(row_index - 1);
            store.col_indices.push_back(col_index - 1);
        }
    } else if constexpr (IS_COMPLEX<Value>) {
        typename Value::value_type real, imag;
        store.values.reserve(stored_nnz);
        for (std::size_t i{0}; i < stored_nnz; ++i) {
            if (!(is >> row_index >> col_index >> real >> imag)) {
                throw std::runtime_error("failed to read complex entry");
            }
            store.row_indices.push_back(row_index - 1);
            store.col_indices.push_back(col_index - 1);
            store.values.emplace_back(real, imag);
        }
    } else {
        Value v;
        store.values.reserve(stored_nnz);
        for (std::size_t i{0}; i < stored_nnz; ++i) {
            if (!(is >> row_index >> col_index >> v)) {
                throw std::runtime_error("failed to read floating point or integral entry");
            }
            store.row_indices.push_back(row_index - 1);
            store.col_indices.push_back(col_index - 1);
            store.values.push_back(v);
        }
    }
    return store;
}

AnyCoo ReadMatrixMarket(std::istream &is) {
    std::string buf;
    if (!std::getline(is, buf)) {
        throw std::runtime_error("bad istream");
    }

    auto tokens{Split<std::vector<std::string>>(buf)};
    if (tokens.size() != 5) {
        throw std::runtime_error("unexpected header tokens number: " + tokens.size());
    }

    if (tokens[0] != "%%MatrixMarket") {
        throw std::runtime_error("unexpected header identifier: " + tokens[0]);
    }

    if (tokens[1] != "matrix") {
        throw std::runtime_error("unexpected object: " + tokens[1]);
    }

    if (tokens[2] != "coordinate" && tokens[2] != "sparse") {
        throw std::runtime_error("unexpected format: " + tokens[2]);
    }

    ValueTypeVar value_var;
    if (tokens[3] == "complex") {
        value_var = meta::Identity<std::complex<double>>{};
    } else if (tokens[3] == "real") {
        value_var = meta::Identity<double>{};
    } else if (tokens[3] == "integer") {
        value_var = meta::Identity<intmax_t>{};
    } else if (tokens[3] == "patten") {
        value_var = meta::Identity<std::monostate>{};
    } else {
        throw std::runtime_error("unexpected value numeric: " + tokens[3]);
    }

    MatrixSymmetric symmetric;
    if (tokens[4] == "general") {
        symmetric = MatrixSymmetric::GENERAL;
    } else if (tokens[4] == "symmetric") {
        symmetric = MatrixSymmetric::SYMMETRIC_LOWER;
    } else if (tokens[4] == "hermitian") {
        if (!std::holds_alternative<meta::Identity<std::complex<double>>>(value_var)) {
            throw std::runtime_error("hermitian without complex");
        }
        symmetric = MatrixSymmetric::HERMITIAN_LOWER;
    } else if (tokens[4] == "skew") {
        symmetric = MatrixSymmetric::SKEW_LOWER;
    } else {
        throw std::runtime_error("unexpected symmetric: " + tokens[4]);
    }

    // skip comments
    while (std::getline(is, buf)) {
        if (!buf.empty() && buf[0] != '%') {
            break;
        }
    }

    std::size_t m, n, stored_nnz;
    if (!(std::istringstream(buf) >> m >> n >> stored_nnz)) {
        throw std::runtime_error("bad matrix dimensions");
    }
    std::cout << "m: " << m << " n: " << n << " stored_nnz: " << stored_nnz << std::endl;

    IndexTypeVar index_var;
    if (m <= std::numeric_limits<int32_t>::max()) {
        index_var = meta::Identity<int32_t>{};
    } else {
        index_var = meta::Identity<int64_t>{};
    }

    return std::visit(
        [&is, m, n, stored_nnz, symmetric](auto value_type, auto index_type) -> AnyCoo {
            using ValueType = typename decltype(value_type)::Type;
            using IndexType = typename decltype(index_type)::Type;
            return Coo<ValueType, IndexType>{
                ReadMatrixMarketStore<ValueType, IndexType>(is, m, n, stored_nnz), symmetric};
        },
        value_var, index_var);
}

template <typename Value, typename DimIndex>
static auto WriteMatrixMarketImpl(std::ostream &os, const Coo<Value, DimIndex> &coo) {
    constexpr MatrixNumeric value_numeric{coo.GetValueNumeric()};
    if constexpr (value_numeric == MatrixNumeric::COMPLEX) {
        os << "complex ";
    } else if constexpr (value_numeric == MatrixNumeric::REAL) {
        os << "real ";
    } else if constexpr (value_numeric == MatrixNumeric::INTEGER) {
        os << "integer ";
    } else if constexpr (value_numeric == MatrixNumeric::PATTERN) {
        os << "patten ";
    } else {
        throw std::runtime_error("unexpected value numeric: " + std::to_string(ToUnderlying(value_numeric)));
    }

    MatrixSymmetric symmectric{coo.GetSymmetric()};
    if (symmectric == MatrixSymmetric::GENERAL) {
        os << "general\n";
    } else if (symmectric == MatrixSymmetric::SYMMETRIC_LOWER) {
        os << "symmetric\n";
    } else if (symmectric == MatrixSymmetric::HERMITIAN_LOWER) {
        os << "hermitian\n";
    } else if (symmectric == MatrixSymmetric::SKEW_LOWER) {
        os << "skew\n";
    } else {
        throw std::runtime_error("unexpected symmetric: " + std::to_string(ToUnderlying(symmectric)));
    }

    os << coo.M() << ' ' << coo.N() << ' ' << coo.StoredNnz() << '\n';

    std::size_t stored_nnz{coo.StoredNnz()};
    const CooStore<Value, DimIndex> &store{coo.GetStore()};
    if constexpr (std::is_same_v<Value, std::monostate>) {
        for (std::size_t i{0}; i < stored_nnz; ++i) {
            os << store.row_indices[i] + 1 << ' ' << store.col_indices[i] + 1 << '\n';
        }
    } else if constexpr (IS_COMPLEX<Value>) {
        for (std::size_t i{0}; i < stored_nnz; ++i) {
            os << store.row_indices[i] + 1 << ' ' << store.col_indices[i] + 1 << ' ' << store.values[i].real() << ' '
               << store.values[i].imag() << '\n';
        }
    } else {
        for (std::size_t i{0}; i < stored_nnz; ++i) {
            os << store.row_indices[i] + 1 << ' ' << store.col_indices[i] + 1 << ' ' << store.values[i] << '\n';
        }
    }
}

void WriteMatrixMarket(std::ostream &os, const AnyCoo &any_coo) {
    os << "%%MatrixMarket matrix coordinate ";
    any_coo.Visit([&os](const auto &coo) { WriteMatrixMarketImpl(os, coo); });
}
} // namespace oops
