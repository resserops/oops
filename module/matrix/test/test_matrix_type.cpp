#include <thread>

#include "oops/matrix_type.h"
#include "gtest/gtest.h"

using namespace oops;

TEST(MatrixType, Convert) {
    int i = 10;
    EXPECT_EQ(Convert<double>(i), 10);

    std::vector<double> v{1.5, 2.6, 3.3};
    auto k = ConvertVector<double>(v);

    for (auto i : k) {
        std::cout << i << " " << i << std::endl;
    }
}

namespace {
struct A {
    A() = default;
    A(const A &) { ++self_copied; }
    A(A &&) { ++self_moved; }

    template <typename T>
    A(const T &) {
        ++other_copied;
    }
    template <typename T>
    A(T &&) {
        ++other_moved;
    }

    inline static void Clear() {
        self_copied = 0;
        self_moved = 0;
        other_copied = 0;
        other_moved = 0;
    }

    inline static size_t self_copied{};
    inline static size_t self_moved{};
    inline static size_t other_copied{};
    inline static size_t other_moved{};
};
} // namespace

TEST(MatrixType, ConvertVectorLVal) {
    std::vector<int> src(2);
    A::Clear();
    ConvertVector<A>(src);
    EXPECT_EQ(A::other_copied, 2);
    EXPECT_EQ(A::other_moved, 0);
    A::Clear();
}

TEST(MatrixType, ConvertVectorRVal) {
    std::vector<int> src(2);
    A::Clear();
    ConvertVector<A>(std::move(src));
    EXPECT_EQ(A::other_copied, 0);
    EXPECT_EQ(A::other_moved, 2);
    A::Clear();
}

TEST(MatrixType, ConvertVectorSameTypeLVal) {
    std::vector<A> src(2);
    A::Clear();
    ConvertVector<A>(src);
    EXPECT_EQ(A::self_copied, 2);
    EXPECT_EQ(A::self_moved, 0);
    A::Clear();
}

TEST(MatrixType, ConvertVectorSameTypeRVal) {
    std::vector<A> src(2);
    A::Clear();
    ConvertVector<A>(std::move(src));
    EXPECT_EQ(A::self_copied, 0);
    EXPECT_EQ(A::self_moved, 0);
    A::Clear();
}
