// SPDX-License-Identifier: GPL-3.0-or-later
#include "numeric.h"
#include <cmath>
#include <cstdio>
#include <limits>
using namespace numeric;
#define CHECK(x) do { if (!(x)) { std::fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #x); return 1; } } while (0)
int main() {
    Registry r; registerBuiltins(r);
    auto mean = r.create("moving_mean"), gaussian = r.create("gaussian_smooth");
    CHECK(mean && gaussian && r.find("moving_mean") && r.find("gaussian_smooth"));
    double values[] = {1,2,3,4,5}, window = 3, alpha = std::sqrt(2*std::log(2.0)), output[5] = {};
    Input in[] = {{values,5,5,1,Type::Float64,Shape::Array}, {&window,1,1,1,Type::Float64,Shape::Scalar},
                  {&alpha,1,1,1,Type::Float64,Shape::Scalar}};
    Output out{output,5,0,Type::Float64,Shape::Array};
    CHECK(mean->execute(in,2,&out,1).status == Status::NotInitialized);
    initializeGsl();
    CHECK(mean->execute(in,2,&out,1).status == Status::Ok && out.count == 5);
    const double expected[] = {4.0/3,2,3,4,14.0/3};
    for (int i=0; i<5; ++i) CHECK(std::fabs(output[i]-expected[i]) < 1e-12);
    double pulse[] = {0,0,1,0,0}; in[0].data = pulse;
    CHECK(gaussian->execute(in,3,&out,1).status == Status::Ok && out.count == 5);
    const double smooth[] = {0,0.25,0.5,0.25,0};
    for (int i=0; i<5; ++i) CHECK(std::fabs(output[i]-smooth[i]) < 1e-12);
    double constant[] = {7,7,7,7,7}; in[0].data = constant;
    CHECK(gaussian->execute(in,3,&out,1).status == Status::Ok);
    for (double v: output) CHECK(std::fabs(v-7) < 1e-12);
    window = 5; CHECK(mean->execute(in,2,&out,1).status == Status::Ok);
    for (double v: output) CHECK(std::fabs(v-7) < 1e-12);
    CHECK(gaussian->execute(in,3,&out,1).status == Status::Ok);
    for (double v: output) CHECK(std::fabs(v-7) < 1e-12);
    window = 3; mean->reset(); gaussian->reset();
    CHECK(mean->execute(in,2,&out,1).status == Status::Ok);
    CHECK(gaussian->execute(in,3,&out,1).status == Status::Ok);
    double inplace[] = {1,2,3,4,5}; in[0].data = inplace; out.data = inplace;
    CHECK(mean->execute(in,2,&out,1).status == Status::Ok);
    for (int i=0; i<5; ++i) CHECK(std::fabs(inplace[i]-expected[i]) < 1e-12);
    double pulseInplace[] = {0,0,1,0,0}; in[0].data = pulseInplace; out.data = pulseInplace;
    CHECK(gaussian->execute(in,3,&out,1).status == Status::Ok);
    for (int i=0; i<5; ++i) CHECK(std::fabs(pulseInplace[i]-smooth[i]) < 1e-12);
    in[0].data = values; out.data = output;
    window = 0; CHECK(mean->execute(in,2,&out,1).status == Status::InvalidArgument && out.count == 0);
    window = 4; CHECK(mean->execute(in,2,&out,1).status == Status::InvalidArgument);
    window = 3.5; CHECK(mean->execute(in,2,&out,1).status == Status::InvalidArgument);
    window = 7; CHECK(mean->execute(in,2,&out,1).status == Status::InvalidArgument);
    window = 4097; CHECK(mean->execute(in,2,&out,1).status == Status::InvalidArgument);
    window = std::numeric_limits<double>::quiet_NaN(); CHECK(mean->execute(in,2,&out,1).status == Status::InvalidArgument);
    window = 3; alpha = 0; CHECK(gaussian->execute(in,3,&out,1).status == Status::InvalidArgument);
    alpha = std::numeric_limits<double>::infinity(); CHECK(gaussian->execute(in,3,&out,1).status == Status::InvalidArgument);
    alpha = 2; in[0].count = 0; CHECK(mean->execute(in,2,&out,1).status == Status::InvalidArgument);
    in[0].count = 5; in[0].capacity = 4; CHECK(mean->execute(in,2,&out,1).status == Status::InvalidArgument);
    in[0].capacity = 5; in[0].stride = 2; CHECK(mean->execute(in,2,&out,1).status == Status::InvalidArgument);
    in[0].stride = 1; out.capacity = 4; CHECK(mean->execute(in,2,&out,1).status == Status::InvalidArgument);
    out.capacity = 5; in[1].shape = Shape::Array; CHECK(mean->execute(in,2,&out,1).status == Status::InvalidArgument);
    in[1].shape = Shape::Scalar;
    out.capacity = 5; out.data = values+1; CHECK(mean->execute(in,2,&out,1).status == Status::InvalidArgument);
    out.data = output; values[2] = std::numeric_limits<double>::quiet_NaN();
    CHECK(mean->execute(in,2,&out,1).status == Status::InvalidArgument);
    values[2] = std::numeric_limits<double>::infinity(); CHECK(gaussian->execute(in,3,&out,1).status == Status::InvalidArgument);
    values[2] = 3;
    double extreme[] = {std::numeric_limits<double>::max(), -std::numeric_limits<double>::max(),
                        std::numeric_limits<double>::max(), 0, 0};
    in[0].data = extreme;
    CHECK(mean->execute(in,2,&out,1).status == Status::NumericalError && out.count == 0);
    in[0].data = values;
    for (int i=0; i<100; ++i) CHECK(mean->execute(in,2,&out,1).status == Status::Ok);
    CHECK(mean->execute(nullptr,0,&out,1).status == Status::InvalidArgument && out.count == 0);
    std::puts("numericWindowTest: all checks passed"); return 0;
}
