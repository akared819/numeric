// SPDX-License-Identifier: GPL-3.0-or-later
#include "numeric.h"
#include <gsl/gsl_math.h>
#include <cmath>

namespace numeric {
namespace {
class LogOnePlus : public Operation {
public:
    void reset() override {}
protected:
    Result executeImpl(const Input* in, std::size_t ni, Output* out, std::size_t no) override {
        if (out) for (std::size_t i = 0; i < no; ++i) out[i].count = 0;
        if (!in || !out || ni != 1 || no != 1) return {Status::InvalidArgument, "log1p requires one input and one output"};
        const Input& x = in[0]; Output& y = out[0];
        if (x.type != Type::Float64 || x.shape != Shape::Scalar || !x.data || x.count != 1 ||
            x.capacity < 1 || x.stride != 1 || y.type != Type::Float64 || y.shape != Shape::Scalar ||
            !y.data || y.capacity < 1 || !std::isfinite(x.data[0]) || x.data[0] <= -1.0)
            return {Status::InvalidArgument, "log1p requires finite x greater than -1 and valid scalar buffers"};
        const double value = gsl_log1p(x.data[0]);
        if (!std::isfinite(value)) return {Status::NumericalError, "non-finite GSL log1p result"};
        y.data[0] = value; y.count = 1;
        return {Status::Ok, "ok"};
    }
};
std::unique_ptr<Operation> makeLogOnePlus() { return std::unique_ptr<Operation>(new LogOnePlus); }
}
void registerElementary(Registry& r) {
    r.add(Metadata{"log1p", "elementary functions", {{Type::Float64, Shape::Scalar}},
                   {{Type::Float64, Shape::Scalar}}, "finite scalar input greater than -1", false,
                   "per-instance object; no workspace", "invalid input or non-finite result"}, &makeLogOnePlus);
}
}
