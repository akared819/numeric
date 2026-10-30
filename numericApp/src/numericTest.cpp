#include "numeric.h"
#include <cmath>
#include <gsl/gsl_errno.h>
#include <cstdio>
#include <limits>
using namespace numeric;
#define CHECK(x) do { if (!(x)) { std::fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #x); return 1; } } while (0)
int main() {
    initializeGsl(); gsl_error("test", __FILE__, __LINE__, GSL_EDOM); Registry r; registerBuiltins(r);
    CHECK(r.find("mean") && !r.find("missing"));
    CHECK(!r.add(*r.find("mean"), nullptr));
    auto op = r.create("mean"); CHECK(op && !r.create("missing"));
    double data[] = {1, 99, 3, 99, 5}; double result = -1;
    Input in{data, 3, 5, 2, Type::Float64, Shape::Array};
    Output out{&result, 1, 0, Type::Float64, Shape::Scalar};
    CHECK(op->execute(&in, 1, &out, 1).status == Status::Ok && out.count == 1 && std::fabs(result - 3) < 1e-12);
    in.count = 1; CHECK(op->execute(&in, 1, &out, 1).status == Status::Ok && result == 1);
    in.count = 0; CHECK(op->execute(&in, 1, &out, 1).status == Status::InvalidArgument && out.count == 0);
    in.count = 3; in.capacity = 4; CHECK(op->execute(&in, 1, &out, 1).status == Status::InvalidArgument);
    in.capacity = 5; in.stride = 0; CHECK(op->execute(&in, 1, &out, 1).status == Status::InvalidArgument);
    in.stride = 2; out.capacity = 0; CHECK(op->execute(&in, 1, &out, 1).status == Status::InvalidArgument);
    out.capacity = 1; in.shape = Shape::Scalar; CHECK(op->execute(&in, 1, &out, 1).status == Status::InvalidArgument);
    in.shape = Shape::Array; data[2] = std::numeric_limits<double>::quiet_NaN(); CHECK(op->execute(&in, 1, &out, 1).status == Status::InvalidArgument);
    data[2] = std::numeric_limits<double>::infinity(); CHECK(op->execute(&in, 1, &out, 1).status == Status::InvalidArgument);
    data[2] = 3; op->reset(); CHECK(op->execute(&in, 1, &out, 1).status == Status::Ok);
    CHECK(op->execute(nullptr, 0, &out, 1).status == Status::InvalidArgument);
    std::puts("numericTest: all checks passed"); return 0;
}
