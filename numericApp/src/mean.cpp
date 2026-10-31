// SPDX-License-Identifier: GPL-3.0-or-later
#include "numeric.h"
#include <gsl/gsl_statistics_double.h>
#include <cmath>
#include <limits>
namespace numeric {
namespace {
class Mean : public Operation {
public:
    Result executeImpl(const Input* in, std::size_t ni, Output* out, std::size_t no) override {
        if (out && no == 1) out[0].count = 0;
        if (!in || !out || ni != 1 || no != 1) return {Status::InvalidArgument, "mean requires one input and one output"};
        const Input& x = in[0]; Output& y = out[0];
        if (x.type != Type::Float64 || x.shape != Shape::Array || y.type != Type::Float64 || y.shape != Shape::Scalar ||
            !x.data || !y.data || !x.count || !x.stride || !y.capacity ||
            (x.count - 1) > (std::numeric_limits<std::size_t>::max() - 1) / x.stride ||
            (x.count - 1) * x.stride >= x.capacity)
            return {Status::InvalidArgument, "invalid buffer, type, count, stride or capacity"};
        for (std::size_t i = 0; i < x.count; ++i)
            if (!std::isfinite(x.data[i * x.stride])) return {Status::InvalidArgument, "input must be finite"};
        const double value = gsl_stats_mean(x.data, x.stride, x.count);
        if (!std::isfinite(value)) return {Status::NumericalError, "non-finite mean"};
        y.data[0] = value; y.count = 1;
        return {Status::Ok, "ok"};
    }
    void reset() override {}
};
std::unique_ptr<Operation> makeMean() { return std::unique_ptr<Operation>(new Mean); }
}
void registerRunningStatistics(Registry&);
void registerBuiltins(Registry& r) {
    r.add(Metadata{"mean", "statistics", {{Type::Float64, Shape::Array}},
                   {{Type::Float64, Shape::Scalar}}, "none", false,
                   "per-instance object; no workspace", "invalid argument or non-finite result"}, &makeMean);
    registerRunningStatistics(r);
}
}
