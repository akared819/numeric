// SPDX-License-Identifier: GPL-3.0-or-later
#include "numeric.h"
#include <cmath>
#include <cstdio>
#include <limits>
using namespace numeric;
#define CHECK(x) do { if (!(x)) { std::fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#x); return 1; } } while (0)
int main() {
    Registry registry; registerBuiltins(registry); auto op=registry.create("polynomial_root"); CHECK(op && registry.find("polynomial_root"));
    double c[]={-2,0,1}, a=0, b=2, value=42;
    Input in[]={{c,3,3,1,Type::Float64,Shape::Array},{&a,1,1,1,Type::Float64,Shape::Scalar},{&b,1,1,1,Type::Float64,Shape::Scalar}};
    Output out{&value,1,0,Type::Float64,Shape::Scalar};
    CHECK(op->execute(in,3,&out,1).status==Status::NotInitialized && value==42);
    initializeGsl();
    CHECK(op->execute(in,3,&out,1).status==Status::Ok && out.count==1 && std::fabs(value-std::sqrt(2.0))<1e-11);
    a=-2; b=0; CHECK(op->execute(in,3,&out,1).status==Status::Ok && std::fabs(value+std::sqrt(2.0))<1e-11);
    c[0]=0; a=0; b=2; CHECK(op->execute(in,3,&out,1).status==Status::Ok && value==0);
    c[0]=-2; a=2; b=3; CHECK(op->execute(in,3,&out,1).status==Status::InvalidArgument && value==0 && out.count==0);
    a=3; b=2; CHECK(op->execute(in,3,&out,1).status==Status::InvalidArgument);
    a=0; b=2; in[0].count=1; CHECK(op->execute(in,3,&out,1).status==Status::InvalidArgument);
    in[0].count=3; in[0].stride=2; CHECK(op->execute(in,3,&out,1).status==Status::InvalidArgument);
    in[0].stride=1; c[1]=std::numeric_limits<double>::quiet_NaN(); CHECK(op->execute(in,3,&out,1).status==Status::InvalidArgument);
    c[1]=0; b=std::numeric_limits<double>::infinity(); CHECK(op->execute(in,3,&out,1).status==Status::InvalidArgument);
    b=2; out.capacity=0; CHECK(op->execute(in,3,&out,1).status==Status::InvalidArgument);
    out.capacity=1; c[2]=std::numeric_limits<double>::max(); CHECK(op->execute(in,3,&out,1).status==Status::NumericalError);
    c[2]=1;
    double difficult[]={-2,0,0,1}; in[0].data=difficult; in[0].count=4; in[0].capacity=4; a=-1e100; b=1e100;
    CHECK(op->execute(in,3,&out,1).status==Status::NumericalError && out.count==0 && value==0);
    in[0].data=c; in[0].count=3; in[0].capacity=3; a=0; b=2;
    op->reset(); for (int i=0;i<100;++i) CHECK(op->execute(in,3,&out,1).status==Status::Ok && std::fabs(value-std::sqrt(2.0))<1e-11);
    std::puts("numericRootsTest: all checks passed"); return 0;
}
