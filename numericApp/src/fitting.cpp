// SPDX-License-Identifier: GPL-3.0-or-later
#include "numeric.h"
#include <gsl/gsl_errno.h>
#include <gsl/gsl_fit.h>
#include <algorithm>
#include <cmath>
#include <limits>

namespace numeric {
namespace {
class LinearFit : public Operation {
public:
    void reset() override {}
protected:
    Result executeImpl(const Input* in, std::size_t ni, Output* out, std::size_t no) override {
        if (out) for (std::size_t i = 0; i < no; ++i) out[i].count = 0;
        if (!in || !out || ni != 2 || no != 1)
            return {Status::InvalidArgument, "linear_fit requires x and y arrays"};
        const Input& x = in[0]; const Input& y = in[1]; Output& result = out[0];
        if (x.type != Type::Float64 || x.shape != Shape::Array || !x.data || x.count < 3 || x.count > x.capacity || x.stride != 1 ||
            y.type != Type::Float64 || y.shape != Shape::Array || !y.data || y.count != x.count || y.count > y.capacity || y.stride != 1 ||
            result.type != Type::Float64 || result.shape != Shape::Array || !result.data || result.capacity < 6)
            return {Status::InvalidArgument, "invalid fitting buffers or dimensions"};
        double minx = x.data[0], maxx = x.data[0], largest = 0;
        for (std::size_t i = 0; i < x.count; ++i) {
            if (!std::isfinite(x.data[i]) || !std::isfinite(y.data[i]))
                return {Status::InvalidArgument, "fit inputs must be finite"};
            minx = std::min(minx, x.data[i]); maxx = std::max(maxx, x.data[i]);
            largest = std::max(largest, std::max(std::fabs(x.data[i]), std::fabs(y.data[i])));
        }
        if (minx == maxx) return {Status::InvalidArgument, "fit x values must not be constant"};
        const double high = std::sqrt(std::numeric_limits<double>::max() / static_cast<double>(x.count)) / 4;
        const double low = 4 * std::sqrt(std::numeric_limits<double>::min());
        if (largest > high || maxx - minx < low)
            return {Status::NumericalError, "fit input exceeds conservative floating-point limits"};
        double v[6];
        if (gsl_fit_linear(x.data, 1, y.data, 1, x.count, &v[0], &v[1], &v[2], &v[3], &v[4], &v[5]) != GSL_SUCCESS)
            return {Status::NumericalError, "GSL fitting failed"};
        for (double value : v) if (!std::isfinite(value)) return {Status::NumericalError, "non-finite fit result"};
        for (std::size_t i = 0; i < 6; ++i) result.data[i] = v[i];
        result.count = 6; return {Status::Ok, "ok"};
    }
};
std::unique_ptr<Operation> makeLinearFit() { return std::unique_ptr<Operation>(new LinearFit); }
}
void registerFitting(Registry& r) {
    const Descriptor array{Type::Float64, Shape::Array};
    r.add(Metadata{"linear_fit", "least squares and fitting", {array, array}, {array},
        "unweighted straight line; output c0,c1,cov00,cov01,cov11,sumsq", false,
        "per-instance object; no workspace", "invalid input, unsafe numeric range or GSL error"}, &makeLinearFit);
}
}
