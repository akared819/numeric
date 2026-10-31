// SPDX-License-Identifier: GPL-3.0-or-later
#include "numeric.h"
#include <cmath>
#include <cstdio>
#include <limits>
using namespace numeric;
#define CHECK(x) do { if (!(x)) { std::fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #x); return 1; } } while (0)
int main() {
    Registry r; registerBuiltins(r);
    auto f = r.create("fft_complex_radix2_forward"), inv = r.create("fft_complex_radix2_inverse");
    CHECK(f && inv && r.find("fft_complex_radix2_forward") && !r.find("fft_complex_radix2_forward")->stateful);
    double signal[] = {1,0, 0,0, 0,0, 0,0}, spectrum[8] = {};
    Input x{signal, 8, 8, 1, Type::Float64, Shape::Array};
    Output y{spectrum, 8, 99, Type::Float64, Shape::Array};
    CHECK(f->execute(&x, 1, &y, 1).status == Status::NotInitialized && y.count == 0);
    initializeGsl();
    CHECK(f->execute(&x, 1, &y, 1).status == Status::Ok && y.count == 8);
    for (int i=0; i<8; i+=2) CHECK(std::fabs(spectrum[i]-1)<1e-12 && std::fabs(spectrum[i+1])<1e-12);
    Input transformed{spectrum, 8, 8, 1, Type::Float64, Shape::Array};
    CHECK(inv->execute(&transformed, 1, &y, 1).status == Status::Ok);
    for (int i=0; i<8; ++i) CHECK(std::fabs(spectrum[i]-signal[i])<1e-12);
    double tone[] = {1,0, 0,1, -1,0, 0,-1}; x.data = tone;
    CHECK(f->execute(&x, 1, &y, 1).status == Status::Ok);
    for (int i=0; i<8; ++i) CHECK(std::fabs(spectrum[i]-(i==2 ? 4.0 : 0.0))<1e-12);
    CHECK(inv->execute(&transformed, 1, &y, 1).status == Status::Ok);
    for (int i=0; i<8; ++i) CHECK(std::fabs(spectrum[i]-tone[i])<1e-12);
    double larger[] = {1,2, 3,4, 5,6, 7,8, 9,10, 11,12, 13,14, 15,16};
    double work[16] = {};
    Input largeIn{larger, 16, 16, 1, Type::Float64, Shape::Array};
    Output largeOut{work, 16, 0, Type::Float64, Shape::Array};
    CHECK(f->execute(&largeIn, 1, &largeOut, 1).status == Status::Ok);
    Input largeSpectrum{work, 16, 16, 1, Type::Float64, Shape::Array};
    CHECK(inv->execute(&largeSpectrum, 1, &largeOut, 1).status == Status::Ok);
    for (int i=0; i<16; ++i) CHECK(std::fabs(work[i]-larger[i])<1e-11);
    x.count = 4; CHECK(f->execute(&x, 1, &y, 1).status == Status::Ok && y.count == 4);
    x.count = 2; CHECK(f->execute(&x, 1, &y, 1).status == Status::Ok && y.count == 2 && spectrum[0] == 1);
    x.count = 0; CHECK(f->execute(&x, 1, &y, 1).status == Status::InvalidArgument && y.count == 0);
    x.count = 3; CHECK(f->execute(&x, 1, &y, 1).status == Status::InvalidArgument);
    x.count = 6; CHECK(f->execute(&x, 1, &y, 1).status == Status::InvalidArgument);
    x.count = 8; x.capacity = 7; CHECK(f->execute(&x, 1, &y, 1).status == Status::InvalidArgument);
    x.capacity = 8; y.capacity = 7; CHECK(f->execute(&x, 1, &y, 1).status == Status::InvalidArgument);
    y.capacity = 8; x.stride = 2; CHECK(f->execute(&x, 1, &y, 1).status == Status::InvalidArgument);
    x.stride = 1; x.shape = Shape::Scalar; CHECK(f->execute(&x, 1, &y, 1).status == Status::InvalidArgument);
    x.shape = Shape::Array; y.shape = Shape::Scalar; CHECK(f->execute(&x, 1, &y, 1).status == Status::InvalidArgument);
    y.shape = Shape::Array; y.data = tone+1; CHECK(f->execute(&x, 1, &y, 1).status == Status::InvalidArgument);
    y.data = spectrum; tone[0] = std::numeric_limits<double>::quiet_NaN();
    CHECK(f->execute(&x, 1, &y, 1).status == Status::InvalidArgument && y.count == 0);
    tone[0] = std::numeric_limits<double>::infinity(); CHECK(f->execute(&x, 1, &y, 1).status == Status::InvalidArgument);
    tone[0] = std::numeric_limits<double>::max(); tone[2] = tone[0];
    CHECK(f->execute(&x, 1, &y, 1).status == Status::NumericalError && y.count == 0);
    tone[0]=1; tone[2]=0;
    for (int i=0; i<100; ++i) CHECK(f->execute(&x, 1, &y, 1).status == Status::Ok);
    CHECK(f->execute(nullptr, 0, &y, 1).status == Status::InvalidArgument && y.count == 0);
    std::puts("numericFftTest: all checks passed");
    return 0;
}
