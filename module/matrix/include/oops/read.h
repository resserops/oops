#pragma once
#include "oops/coo.h"
#include "oops/meta_type_list.h"
#include "oops/str.h"
#include <complex>
#include <fstream>
#include <variant>

using namespace oops::meta;
namespace oops {
template <typename TL>
using ApplyToCoo = ApplyT<Coo, TL>;
using CooVar = ApplyT<std::variant, TransformT<ApplyToCoo, CartProdT<ValueType, IndexType>>>;

template <typename Value>
struct CooConstructor {
    template <typename DimIndex>
    CooVar Gen() {
        return Coo<Value, DimIndex>{};
    }
};

using CooConstructorVar = ApplyT<std::variant, TransformT<CooConstructor, ValueType>>;

CooVar ReadMatrixMarket(std::istream &is) {
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

    CooConstructorVar var0;
    std::cout << tokens[3] << std::endl;
    if (tokens[3] == "complex") {
        var0 = CooConstructor<std::complex<double>>{};
    } else if (tokens[3] == "real") {
        var0 = CooConstructor<double>{};
    } else if (tokens[3] == "integer") {
        var0 = CooConstructor<intmax_t>{};
    } else if (tokens[3] == "patten") {
        var0 = CooConstructor<std::monostate>{};
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

    CooVar coo_var;
    if (m <= std::numeric_limits<int32_t>::max()) {
        coo_var = std::visit([](auto &&constructor) { return constructor.template Gen<int32_t>(); }, var0);
    } else {
        coo_var = std::visit([](auto &&constructor) { return constructor.template Gen<int64_t>(); }, var0);
    }

    std::visit(
        [m, n, nnz, &is](auto &coo) {
            using ValueType = typename std::decay_t<decltype(coo)>::ValueType;
            using DimIndexType = typename std::decay_t<decltype(coo)>::DimIndexType;
            using StoreType = typename std::decay_t<decltype(coo)>::StoreType;
            StoreType store;
            store.m = m;
            store.n = n;

            DimIndexType i, j;
            store.row_idx.reserve(nnz);
            store.col_idx.reserve(nnz);
            if constexpr (std::is_same_v<ValueType, std::monostate>) {
                for (std::size_t k{0}; k < nnz; ++k) {
                    if (!(is >> i >> j)) {
                        throw std::runtime_error("Failed to read entry");
                    }
                    store.row_idx.push_back(i - 1);
                    store.col_idx.push_back(j - 1);
                }
            } else if constexpr (std::is_same_v<ValueType, std::complex<double>>) {
                double real, imag;
                store.values.reserve(nnz);
                for (std::size_t k{0}; k < nnz; ++k) {
                    if (!(is >> i >> j >> real >> imag)) {
                        throw std::runtime_error("Failed to read entry");
                    }
                    store.row_idx.push_back(i - 1);
                    store.col_idx.push_back(j - 1);
                    store.values.emplace_back(real, imag);
                }
            } else {
                ValueType v;
                store.values.reserve(nnz);
                for (std::size_t k{0}; k < nnz; ++k) {
                    if (!(is >> i >> j >> v)) {
                        throw std::runtime_error("Failed to read entry");
                    }
                    store.row_idx.push_back(i - 1);
                    store.col_idx.push_back(j - 1);
                    store.values.push_back(v);
                }
            }
            coo = std::move(store);
        },
        coo_var);
    return coo_var;
}
} // namespace oops
