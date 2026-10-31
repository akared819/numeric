// SPDX-License-Identifier: GPL-3.0-or-later
#include "numeric.h"
#include <cmath>
#include <cstdio>
#include <limits>
using namespace numeric;
#define CHECK(x) do { if (!(x)) { std::fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #x); return 1; } } while (0)
int main() {
    Registry registry; registerBuiltins(registry);
    auto fit = registry.create("linear_fit"); CHECK(fit && registry.find("linear_fit"));
    double x[] = {0,1,2,3}, y[] = {1,2,2,4}, v[] = {42,42,42,42,42,42};
    Input in[] = {{x,4,4,1,Type::Float64,Shape::Array}, {y,4,4,1,Type::Float64,Shape::Array}};
    Output out{v,6,0,Type::Float64,Shape::Array};
    CHECK(fit->execute(in,2,&out,1).status == Status::NotInitialized && v[0] == 42);
    initializeGsl();
    CHECK(fit->execute(in,2,&out,1).status == Status::Ok && out.count == 6);
    const double expected[] = {0.9,0.9,0.245,-0.105,0.07,0.7};
    for (int i=0;i<6;++i) CHECK(std::fabs(v[i]-expected[i]) < 1e-12);
    in[0].count=2; CHECK(fit->execute(in,2,&out,1).status == Status::InvalidArgument && out.count == 0 && std::fabs(v[0]-expected[0]) < 1e-12);
    in[0].count=4; in[1].count=3; CHECK(fit->execute(in,2,&out,1).status == Status::InvalidArgument);
    in[1].count=4; out.capacity=5; CHECK(fit->execute(in,2,&out,1).status == Status::InvalidArgument);
    out.capacity=6; in[0].stride=2; CHECK(fit->execute(in,2,&out,1).status == Status::InvalidArgument);
    in[0].stride=1; y[0]=std::numeric_limits<double>::quiet_NaN(); CHECK(fit->execute(in,2,&out,1).status == Status::InvalidArgument);
    y[0]=1; x[1]=0; x[2]=0; x[3]=0; CHECK(fit->execute(in,2,&out,1).status == Status::InvalidArgument);
    x[1]=1; x[2]=2; x[3]=3;
    x[3]=std::numeric_limits<double>::max(); CHECK(fit->execute(in,2,&out,1).status == Status::NumericalError);
    x[1]=1e-300; x[2]=2e-300; x[3]=3e-300; CHECK(fit->execute(in,2,&out,1).status == Status::NumericalError);
    x[1]=1; x[2]=2; x[3]=3; fit->reset();
    for(int i=0;i<100;++i) CHECK(fit->execute(in,2,&out,1).status == Status::Ok && std::fabs(v[1]-.9)<1e-12);
    std::puts("numericFitTest: all checks passed"); return 0;
}
