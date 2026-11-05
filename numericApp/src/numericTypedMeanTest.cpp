// SPDX-License-Identifier: GPL-3.0-or-later
#include "numericTypedMean.h"
#include <cmath>
#include <cstdio>
#include <limits>
using namespace numeric;
#define CHECK(x) do { if (!(x)) { std::fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #x); return 1; } } while (0)
int main() {
    const std::size_t shape[] = {4}, fStride[] = {sizeof(float)}, dStride[] = {sizeof(double)},
                      iStride[] = {sizeof(std::int32_t)}, lStride[] = {sizeof(std::int64_t)},
                      c64Stride[] = {2 * sizeof(float)}, c128Stride[] = {2 * sizeof(double)};
    float f[] = {1, 2, 3, 4}; double d[] = {1, 2, 3, 4};
    std::int32_t i[] = {1, 2, 3, 4}; std::int64_t l[] = {1, 2, 3, 4};
    float c64[] = {1, 0, 2, 0, 3, 0, 4, 0}; double c128[] = {1, 0, 2, 0, 3, 0, 4, 0};
    ArrayView views[] = {{f, sizeof(f), ElementType::Float32, 1, shape, fStride},
                         {d, sizeof(d), ElementType::Float64, 1, shape, dStride},
                         {i, sizeof(i), ElementType::Int32, 1, shape, iStride},
                         {l, sizeof(l), ElementType::Int64, 1, shape, lStride},
                         {c64, sizeof(c64), ElementType::Complex64, 1, shape, c64Stride},
                         {c128, sizeof(c128), ElementType::Complex128, 1, shape, c128Stride}};
    TypedMean mean(8); double result = 77;
    CHECK(mean.execute(views[0], result).status == Status::NotInitialized && result == 77);
    initializeGsl(); CHECK(mean.reserve().status == Status::Ok);
    for (const auto& view : views) CHECK(mean.execute(view, result).status == Status::Ok && std::fabs(result - 2.5) < 1e-12);
    double padded[] = {1, 2, 999, 3, 4, 999};
    const std::size_t matrixShape[] = {2, 2}, matrixStrides[] = {3 * sizeof(double), sizeof(double)};
    ArrayView matrix{padded, sizeof(padded), ElementType::Float64, 2, matrixShape, matrixStrides};
    CHECK(mean.execute(matrix, result).status == Status::Ok && std::fabs(result - 2.5) < 1e-12);
    c128[1] = 1; result = 77;
    CHECK(mean.execute(views[5], result).status == Status::InvalidArgument && result == 77); c128[1] = 0;
    l[0] = INT64_C(9007199254740993);
    CHECK(mean.execute(views[3], result).status == Status::InvalidArgument && result == 77); l[0] = 1;
    d[0] = std::numeric_limits<double>::quiet_NaN();
    CHECK(mean.execute(views[1], result).status == Status::InvalidArgument && result == 77); d[0] = 1;
    const std::size_t emptyShape[] = {0}; ArrayView empty{nullptr, 0, ElementType::Float64, 1, emptyShape, dStride};
    CHECK(mean.execute(empty, result).status == Status::InvalidArgument && result == 77);
    TypedMean limited(3); CHECK(limited.execute(views[0], result).status == Status::InvalidArgument && result == 77);
    TypedMean zero(0); CHECK(zero.reserve().status == Status::InvalidArgument);
    mean.reset(); CHECK(mean.execute(views[0], result).status == Status::Ok && std::fabs(result - 2.5) < 1e-12);
    ArrayView scalar{d, sizeof(double), ElementType::Float64, 0, nullptr, nullptr};
    CHECK(mean.execute(scalar, result).status == Status::Ok && result == 1);
    std::puts("numericTypedMeanTest: all checks passed"); return 0;
}
