// SPDX-License-Identifier: GPL-3.0-or-later
#include "numeric.h"
#include <cmath>
#include <cstdio>
#include <limits>
using namespace numeric;
#define CHECK(x) do { if (!(x)) { std::fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#x); return 1; } } while (0)
int main() {
    Registry registry; registerBuiltins(registry); auto op=registry.create("polynomial_integral"); CHECK(op && registry.find("polynomial_integral"));
    double c[]={2,3,4}, a=0, b=2, result=42;
    Input in[]={{c,3,3,1,Type::Float64,Shape::Array},{&a,1,1,1,Type::Float64,Shape::Scalar},{&b,1,1,1,Type::Float64,Shape::Scalar}};
    Output out{&result,1,0,Type::Float64,Shape::Scalar};
    CHECK(op->execute(in,3,&out,1).status==Status::NotInitialized && result==42);
    initializeGsl();
    CHECK(op->execute(in,3,&out,1).status==Status::Ok && out.count==1 && std::fabs(result-62.0/3)<1e-12);
    a=2; b=0; CHECK(op->execute(in,3,&out,1).status==Status::Ok && std::fabs(result+62.0/3)<1e-12);
    a=1; b=1; CHECK(op->execute(in,3,&out,1).status==Status::Ok && result==0);
    in[0].count=0; CHECK(op->execute(in,3,&out,1).status==Status::InvalidArgument && result==0 && out.count==0);
    in[0].count=3; in[0].stride=2; CHECK(op->execute(in,3,&out,1).status==Status::InvalidArgument);
    in[0].stride=1; c[1]=std::numeric_limits<double>::quiet_NaN(); CHECK(op->execute(in,3,&out,1).status==Status::InvalidArgument);
    c[1]=3; b=std::numeric_limits<double>::infinity(); CHECK(op->execute(in,3,&out,1).status==Status::InvalidArgument);
    b=2; a=0; out.capacity=0; CHECK(op->execute(in,3,&out,1).status==Status::InvalidArgument);
    out.capacity=1; c[2]=std::numeric_limits<double>::max(); CHECK(op->execute(in,3,&out,1).status==Status::NumericalError);
    c[2]=4;
    double highDegree[64] = {}; highDegree[62] = 1; a=-1; b=1;
    Input highIn[]={{highDegree,64,64,1,Type::Float64,Shape::Array},in[1],in[2]};
    CHECK(op->execute(highIn,3,&out,1).status==Status::Ok && std::fabs(result-2.0/63)<1e-14);
    highIn[0].count=65; highIn[0].capacity=65;
    CHECK(op->execute(highIn,3,&out,1).status==Status::InvalidArgument);
    a=0; b=2; op->reset(); for (int i=0;i<100;++i) CHECK(op->execute(in,3,&out,1).status==Status::Ok && std::fabs(result-62.0/3)<1e-12);
    std::puts("numericIntegrationTest: all checks passed"); return 0;
}
