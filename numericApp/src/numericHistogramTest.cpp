// SPDX-License-Identifier: GPL-3.0-or-later
#include "numeric.h"
#include <cmath>
#include <cstdio>
#include <limits>
using namespace numeric;
#define CHECK(x) do { if (!(x)) { std::fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #x); return 1; } } while (0)
int main() {
    Registry registry; registerBuiltins(registry);
    auto op = registry.create("histogram_uniform"), other = registry.create("histogram_uniform");
    CHECK(op && other && registry.find("histogram_uniform") && registry.find("histogram_uniform")->inputs.size() == 4);
    double samples[] = {0,0.25,0.5,0.75,0.999}, lower = 0, upper = 1, bins = 4, counts[4] = {9,9,9,9};
    Input in[] = {{samples,5,5,1,Type::Float64,Shape::Array}, {&lower,1,1,1,Type::Float64,Shape::Scalar},
                  {&upper,1,1,1,Type::Float64,Shape::Scalar}, {&bins,1,1,1,Type::Float64,Shape::Scalar}};
    Output out{counts,4,0,Type::Float64,Shape::Array};
    CHECK(op->execute(in,4,&out,1).status == Status::NotInitialized && out.count == 0 && counts[0] == 9);
    initializeGsl();
    CHECK(op->execute(in,4,&out,1).status == Status::Ok && out.count == 4 && counts[0] == 1 && counts[1] == 1 && counts[2] == 1 && counts[3] == 2);
    bins = 2; CHECK(op->execute(in,4,&out,1).status == Status::Ok && out.count == 2 && counts[0] == 2 && counts[1] == 3);
    bins = 4; CHECK(other->execute(in,4,&out,1).status == Status::Ok && counts[3] == 2);
    CHECK(op->execute(in,4,&out,1).status == Status::Ok && counts[3] == 2);
    op->reset(); CHECK(op->execute(in,4,&out,1).status == Status::Ok && counts[3] == 2);
    samples[4] = 1; CHECK(op->execute(in,4,&out,1).status == Status::InvalidArgument && out.count == 0 && counts[3] == 2);
    samples[4] = -0.1; CHECK(op->execute(in,4,&out,1).status == Status::InvalidArgument);
    samples[4] = std::numeric_limits<double>::quiet_NaN(); CHECK(op->execute(in,4,&out,1).status == Status::InvalidArgument);
    samples[4] = std::numeric_limits<double>::infinity(); CHECK(op->execute(in,4,&out,1).status == Status::InvalidArgument);
    samples[4] = 0.999; bins = 0; CHECK(op->execute(in,4,&out,1).status == Status::InvalidArgument);
    bins = 2.5; CHECK(op->execute(in,4,&out,1).status == Status::InvalidArgument);
    bins = 65537; CHECK(op->execute(in,4,&out,1).status == Status::InvalidArgument);
    bins = 4; out.capacity = 3; CHECK(op->execute(in,4,&out,1).status == Status::InvalidArgument);
    out.capacity = 4; lower = 1; CHECK(op->execute(in,4,&out,1).status == Status::InvalidArgument);
    lower = -std::numeric_limits<double>::max(); upper = std::numeric_limits<double>::max();
    CHECK(op->execute(in,4,&out,1).status == Status::InvalidArgument);
    lower = 1; upper = std::nextafter(1.0, 2.0); bins = 2; samples[0] = 1;
    in[0].count = 1; CHECK(op->execute(in,4,&out,1).status == Status::NumericalError && out.count == 0);
    lower = 0; upper = 1; bins = 4; samples[0] = 0; in[0].count = 0;
    CHECK(op->execute(in,4,&out,1).status == Status::InvalidArgument);
    in[0].count = 5; in[0].capacity = 4; CHECK(op->execute(in,4,&out,1).status == Status::InvalidArgument);
    in[0].capacity = 5; in[0].stride = 2; CHECK(op->execute(in,4,&out,1).status == Status::InvalidArgument);
    in[0].stride = 1; in[1].shape = Shape::Array; CHECK(op->execute(in,4,&out,1).status == Status::InvalidArgument);
    in[1].shape = Shape::Scalar; CHECK(op->execute(nullptr,0,&out,1).status == Status::InvalidArgument && out.count == 0);
    for (int i=0; i<100; ++i) CHECK(op->execute(in,4,&out,1).status == Status::Ok && counts[3] == 2);
    std::puts("numericHistogramTest: all checks passed"); return 0;
}
