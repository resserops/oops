#include "oops/matrix_market_io.h"

#include "oops/str.h"

namespace oops {
template <typename Value, typename DimIndex>
static Coo<Value, DimIndex> ReadMatrixMarketStore(std::istream &is, std::size_t m, std::size_t n, std::size_t nnz) {
    CooStore<Value, DimIndex> store;
    store.m = m;
    store.n = n;

    DimIndex i, j;
    store.row_indices.reserve(nnz);
    store.col_indices.reserve(nnz);
    if constexpr (std::is_same_v<Value, std::monostate>) {
        for (std::size_t k{0}; k < nnz; ++k) {
            if (!(is >> i >> j)) {
                throw std::runtime_error("failed to read pattern entry");
            }
            store.row_indices.push_back(i - 1);
            store.col_indices.push_back(j - 1);
        }
    } else if constexpr (IS_COMPLEX<Value>) {
        typename Value::value_type real, imag;
        store.values.reserve(nnz);
        for (std::size_t k{0}; k < nnz; ++k) {
            if (!(is >> i >> j >> real >> imag)) {
                throw std::runtime_error("failed to read complex entry");
            }
            store.row_indices.push_back(i - 1);
            store.col_indices.push_back(j - 1);
            store.values.emplace_back(real, imag);
        }
    } else {
        Value v;
        store.values.reserve(nnz);
        for (std::size_t k{0}; k < nnz; ++k) {
            if (!(is >> i >> j >> v)) {
                throw std::runtime_error("failed to read floating point or integral entry");
            }
            store.row_indices.push_back(i - 1);
            store.col_indices.push_back(j - 1);
            store.values.push_back(v);
        }
    }
    return Coo<Value, DimIndex>{store};
}

AnyCoo ReadMatrixMarket(std::istream &is) {
    std::string buf;
    if (!std::getline(is, buf)) {
        throw std::runtime_error("Empty istream");
    }

    std::string header, object, format, numeric, symmetry;
    auto tokens{Split<std::vector<std::string>>(buf)};
    if (tokens.size() != 5) {
        throw std::runtime_error("Invalid header");
    }

    if (tokens[0] != "%%MatrixMarket") {
        throw std::runtime_error("Invalid header");
    }

    if (tokens[1] != "matrix") {
        throw std::runtime_error("unexpect object");
    }

    if (tokens[2] != "coordinate" && tokens[2] != "sparse") {
        throw std::runtime_error("not support dense");
    }

    while (std::getline(is, buf)) {
        if (!buf.empty() && buf[0] != '%') {
            break;
        }
    }

    std::size_t m, n, nnz;
    if (!(std::istringstream(buf) >> m >> n >> nnz)) {
        throw std::runtime_error("Failed to read matrix dimensions");
    }
    std::cout << "m: " << m << " n: " << n << " nnz: " << nnz << std::endl;

    std::cout << tokens[3] << std::endl;
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
        throw std::runtime_error("unexpected value type");
    }

    IndexTypeVar index_var;
    if (m <= std::numeric_limits<int32_t>::max()) {
        index_var = meta::Identity<int32_t>{};
    } else {
        index_var = meta::Identity<int64_t>{};
    }

    return std::visit(
        [&](auto value_type, auto index_type) {
            using ValueType = typename decltype(value_type)::Type;
            using IndexType = typename decltype(index_type)::Type;
            return AnyCoo{ReadMatrixMarketStore<ValueType, IndexType>(is, m, n, nnz)};
        },
        value_var, index_var);
}
} // namespace oops
