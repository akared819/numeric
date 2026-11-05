// SPDX-License-Identifier: GPL-3.0-or-later
#include "numericData.h"
#include <cmath>
#include <cstdio>
#include <cstring>
#include <limits>
using namespace numeric;
#define CHECK(x) do { if (!(x)) { std::fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #x); return 1; } } while (0)
int main() {
    const std::size_t shape[] = {2, 3}, strides[] = {32, 8};
    unsigned char storage[64] = {};
    MutableArrayView integers{storage, sizeof(storage), ElementType::Int64, 2, shape, strides};
    const ArrayView readInts{storage, sizeof(storage), ElementType::Int64, 2, shape, strides};
    CHECK(validateWritableArray(integers).status == Status::Ok);
    const std::size_t index[] = {1, 2};
    ElementValue input{ElementType::Int64, INT64_C(9007199254740993), 0, 0}, output{};
    CHECK(writeElement(integers, index, 2, input).status == Status::Ok);
    CHECK(readElement(readInts, index, 2, output).status == Status::Ok && output.integer == input.integer);
    std::int64_t actual = 0; std::memcpy(&actual, storage + 48, sizeof(actual)); CHECK(actual == input.integer);
    const std::size_t outside[] = {2, 0};
    CHECK(readElement(readInts, outside, 2, output).status == Status::InvalidArgument);
    CHECK(writeElement(integers, nullptr, 2, input).status == Status::InvalidArgument);
    CHECK(writeElement(integers, index, 1, input).status == Status::InvalidArgument);
    MutableArrayView small = integers; small.capacityBytes = 55;
    CHECK(validateWritableArray(small).status == Status::InvalidArgument);
    const std::size_t overlap[] = {8, 8}; MutableArrayView overlapping = integers; overlapping.stridesBytes = overlap;
    CHECK(validateWritableArray(overlapping).status == Status::InvalidArgument);
    ArrayView overlappingRead{storage, sizeof(storage), ElementType::Int64, 2, shape, overlap};
    CHECK(validateArray(overlappingRead).status == Status::Ok);
    const std::size_t huge[] = {std::numeric_limits<std::size_t>::max(), 3};
    ArrayView overflow{storage, sizeof(storage), ElementType::Int64, 2, huge, strides};
    CHECK(validateArray(overflow).status == Status::InvalidArgument);
    const std::size_t zeros[] = {0, 3}; ArrayView empty{nullptr, 0, ElementType::Float64, 2, zeros, strides};
    CHECK(validateArray(empty).status == Status::Ok);
    CHECK(readElement(empty, index, 2, output).status == Status::InvalidArgument);
    ArrayView bad = readInts; bad.rank = 9; CHECK(validateArray(bad).status == Status::InvalidArgument);
    bad = readInts; bad.type = static_cast<ElementType>(99); CHECK(validateArray(bad).status == Status::InvalidArgument);

    float f32 = 0; MutableArrayView floatOut{&f32, sizeof(f32), ElementType::Float32, 0, nullptr, nullptr};
    ArrayView floatIn{&f32, sizeof(f32), ElementType::Float32, 0, nullptr, nullptr};
    ElementValue f{ElementType::Float32, 0, 1.25, 0};
    CHECK(writeElement(floatOut, nullptr, 0, f).status == Status::Ok && f32 == 1.25f);
    CHECK(readElement(floatIn, nullptr, 0, output).status == Status::Ok && output.real == 1.25);
    f.real = 0.1; CHECK(writeElement(floatOut, nullptr, 0, f).status == Status::InvalidArgument && f32 == 1.25f);
    double f64 = 0; MutableArrayView doubleOut{&f64, sizeof(f64), ElementType::Float64, 0, nullptr, nullptr};
    ArrayView doubleIn{&f64, sizeof(f64), ElementType::Float64, 0, nullptr, nullptr};
    ElementValue d{ElementType::Float64, 0, 0.1, 0};
    CHECK(writeElement(doubleOut, nullptr, 0, d).status == Status::Ok && f64 == 0.1);
    CHECK(readElement(doubleIn, nullptr, 0, output).status == Status::Ok && output.real == 0.1);
    d.real = std::numeric_limits<double>::infinity();
    CHECK(writeElement(doubleOut, nullptr, 0, d).status == Status::InvalidArgument && f64 == 0.1);
    std::int32_t i32 = 0; MutableArrayView intOut{&i32, sizeof(i32), ElementType::Int32, 0, nullptr, nullptr};
    ArrayView intIn{&i32, sizeof(i32), ElementType::Int32, 0, nullptr, nullptr};
    ElementValue iv{ElementType::Int32, -123, 0, 0};
    CHECK(writeElement(intOut, nullptr, 0, iv).status == Status::Ok && i32 == -123);
    CHECK(readElement(intIn, nullptr, 0, output).status == Status::Ok && output.integer == -123);
    iv.integer = INT64_C(2147483648); CHECK(writeElement(intOut, nullptr, 0, iv).status == Status::InvalidArgument && i32 == -123);
    unsigned char unaligned[17] = {}; MutableArrayView complexOut{unaligned + 1, 16, ElementType::Complex128, 0, nullptr, nullptr};
    ArrayView complexIn{unaligned + 1, 16, ElementType::Complex128, 0, nullptr, nullptr};
    ElementValue complex{ElementType::Complex128, 0, 1.5, -2.25};
    CHECK(writeElement(complexOut, nullptr, 0, complex).status == Status::Ok);
    CHECK(readElement(complexIn, nullptr, 0, output).status == Status::Ok && output.real == 1.5 && output.imaginary == -2.25);
    double components[2]; std::memcpy(components, unaligned + 1, sizeof(components));
    CHECK(components[0] == 1.5 && components[1] == -2.25);
    unsigned char singleComplex[8] = {}; MutableArrayView c64out{singleComplex, 8, ElementType::Complex64, 0, nullptr, nullptr};
    ArrayView c64in{singleComplex, 8, ElementType::Complex64, 0, nullptr, nullptr};
    ElementValue c64{ElementType::Complex64, 0, 2.5, -3.25};
    CHECK(writeElement(c64out, nullptr, 0, c64).status == Status::Ok);
    CHECK(readElement(c64in, nullptr, 0, output).status == Status::Ok && output.real == 2.5 && output.imaginary == -3.25);
    c64.imaginary = 0.1; CHECK(writeElement(c64out, nullptr, 0, c64).status == Status::InvalidArgument);
    CHECK(readElement(c64in, nullptr, 0, output).status == Status::Ok && output.imaginary == -3.25);
    ElementValue converted{ElementType::Int64, 77, 0, 0};
    ElementValue large{ElementType::Int64, INT64_C(9007199254740993), 0, 0};
    CHECK(convertElement(large, ElementType::Float64, converted).status == Status::InvalidArgument && converted.integer == 77);
    large.integer = INT64_C(9007199254740994);
    CHECK(convertElement(large, ElementType::Float64, converted).status == Status::Ok && converted.real == 9007199254740994.0);
    large.integer = std::numeric_limits<std::int64_t>::min();
    CHECK(convertElement(large, ElementType::Float64, converted).status == Status::Ok && converted.real == -9223372036854775808.0);
    large.integer = std::numeric_limits<std::int64_t>::max();
    CHECK(convertElement(large, ElementType::Float64, converted).status == Status::InvalidArgument);
    large.integer = 16777217; CHECK(convertElement(large, ElementType::Float32, converted).status == Status::InvalidArgument);
    large.integer = 16777218; CHECK(convertElement(large, ElementType::Float32, converted).status == Status::Ok);
    ElementValue fractional{ElementType::Float64, 0, 1.5, 0};
    CHECK(convertElement(fractional, ElementType::Int64, converted).status == Status::InvalidArgument);
    fractional.real = 9223372036854775808.0;
    CHECK(convertElement(fractional, ElementType::Int64, converted).status == Status::InvalidArgument);
    fractional.real = -9223372036854775808.0;
    CHECK(convertElement(fractional, ElementType::Int64, converted).status == Status::Ok && converted.integer == std::numeric_limits<std::int64_t>::min());
    ElementValue complexValue{ElementType::Complex128, 0, 2.5, 1};
    CHECK(convertElement(complexValue, ElementType::Float64, converted).status == Status::InvalidArgument);
    complexValue.imaginary = 0;
    CHECK(convertElement(complexValue, ElementType::Float32, converted).status == Status::Ok && converted.real == 2.5);
    CHECK(convertElement(complexValue, ElementType::Complex64, converted).status == Status::Ok && converted.imaginary == 0);
    fractional.real = 0.1; CHECK(convertElement(fractional, ElementType::Float32, converted).status == Status::InvalidArgument);
    fractional.real = std::numeric_limits<double>::quiet_NaN();
    CHECK(convertElement(fractional, ElementType::Float64, converted).status == Status::InvalidArgument);
    std::puts("numericDataTest: all checks passed"); return 0;
}
