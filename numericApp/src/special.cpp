// SPDX-License-Identifier: GPL-3.0-or-later
#include "numeric.h"
#include <gsl/gsl_errno.h>
#include <gsl/gsl_sf_erf.h>
#include <gsl/gsl_sf_gamma.h>
#include <cmath>

namespace numeric {
namespace {
class GammaFunction : public Operation {
public:
    void reset() override {}
protected:
    Result executeImpl(const Input* in, std::size_t ni, Output* out, std::size_t no) override {
        if (out && no == 1) out[0].count = 0;
        if (!in || !out || ni != 1 || no != 1) return {Status::InvalidArgument, "gamma requires one input and output"};
        const Input& x = in[0]; Output& y = out[0];
        if (x.type != Type::Float64 || x.shape != Shape::Scalar || !x.data || x.count != 1 || !x.capacity || x.stride != 1 ||
            y.type != Type::Float64 || y.shape != Shape::Scalar || !y.data || !y.capacity || !std::isfinite(x.data[0]) ||
            (x.data[0] <= 0 && std::floor(x.data[0]) == x.data[0]))
            return {Status::InvalidArgument, "invalid gamma input"};
        gsl_sf_result value;
        if (gsl_sf_gamma_e(x.data[0], &value) != GSL_SUCCESS || !std::isfinite(value.val) || !std::isfinite(value.err))
            return {Status::NumericalError, "GSL gamma failed or overflowed"};
        y.data[0] = value.val; y.count = 1; return {Status::Ok, "ok"};
    }
};
class ErrorFunction : public Operation {
public:
    void reset() override {}
protected:
    Result executeImpl(const Input* in, std::size_t ni, Output* out, std::size_t no) override {
        if (out) for (std::size_t i = 0; i < no; ++i) out[i].count = 0;
        if (!in || !out || ni != 1 || no != 1) return {Status::InvalidArgument, "erf requires one input and one output"};
        const Input& x = in[0]; Output& y = out[0];
        if (x.type != Type::Float64 || x.shape != Shape::Scalar || !x.data || x.count != 1 ||
            x.capacity < 1 || x.stride != 1 || y.type != Type::Float64 || y.shape != Shape::Scalar ||
            !y.data || y.capacity < 1 || !std::isfinite(x.data[0]))
            return {Status::InvalidArgument, "invalid erf input or output"};
        gsl_sf_result value;
        const int status = gsl_sf_erf_e(x.data[0], &value);
        if (status != GSL_SUCCESS || !std::isfinite(value.val) || !std::isfinite(value.err))
            return {Status::NumericalError, "GSL erf failed or returned a non-finite result"};
        y.data[0] = value.val; y.count = 1;
        return {Status::Ok, "ok"};
    }
};
std::unique_ptr<Operation> makeErf() { return std::unique_ptr<Operation>(new ErrorFunction); }
std::unique_ptr<Operation> makeGamma() { return std::unique_ptr<Operation>(new GammaFunction); }
}
void registerSpecialFunctions(Registry& r) {
    r.addWithoutArguments(Metadata{"gamma", "special functions", {{Type::Float64, Shape::Scalar}},
                   {{Type::Float64, Shape::Scalar}}, "finite scalar except non-positive integers", false,
                   "per-instance object; no workspace", "invalid argument or GSL numerical error"}, &makeGamma);
    r.addWithoutArguments(Metadata{"erf", "special functions", {{Type::Float64, Shape::Scalar}},
                   {{Type::Float64, Shape::Scalar}}, "finite scalar input", false,
                   "per-instance object; no workspace", "invalid argument or GSL numerical error"}, &makeErf);
}
}
