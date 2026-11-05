// SPDX-License-Identifier: GPL-3.0-or-later
#include "numeric.h"
#include <gsl/gsl_errno.h>
#include <gsl/gsl_fit.h>
#include <gsl/gsl_multifit_nlinear.h>
#include <gsl/gsl_blas.h>
#include <algorithm>
#include <cmath>
#include <limits>

namespace numeric {
namespace {
struct ResidualData {
    ResidualCallback callback; void* context; std::vector<double> parameters, residuals; bool failed;
};
int invokeResidual(const gsl_vector* x, void* raw, gsl_vector* output) {
    ResidualData& data = *static_cast<ResidualData*>(raw);
    for (std::size_t i = 0; i < data.parameters.size(); ++i) {
        data.parameters[i] = gsl_vector_get(x, i);
        if (!std::isfinite(data.parameters[i])) { data.failed = true; return GSL_EBADFUNC; }
    }
    std::fill(data.residuals.begin(), data.residuals.end(), std::numeric_limits<double>::quiet_NaN());
    try {
        if (!data.callback(data.parameters.data(), data.parameters.size(), data.residuals.data(), data.residuals.size(), data.context)) {
            data.failed = true; return GSL_EBADFUNC;
        }
    } catch (...) { data.failed = true; return GSL_EBADFUNC; }
    for (std::size_t i = 0; i < data.residuals.size(); ++i) {
        if (!std::isfinite(data.residuals[i])) { data.failed = true; return GSL_EBADFUNC; }
        gsl_vector_set(output, i, data.residuals[i]);
    }
    return GSL_SUCCESS;
}
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
Result fitNonlinear(ResidualCallback callback, void* context, const double* initial, std::size_t p,
                    std::size_t n, std::size_t maxIterations, double tolerance,
                    double* parameters, std::size_t capacity, double& sumSquaredResiduals) {
    if (!isGslInitialized()) return {Status::NotInitialized, "GSL policy not initialized"};
    if (!callback || !initial || !parameters || !p || p > 64 || n < p || n > 10000 || capacity < p ||
        !maxIterations || maxIterations > 1000000 || !std::isfinite(tolerance) || tolerance <= 0)
        return {Status::InvalidArgument, "invalid nonlinear fit inputs"};
    for (std::size_t i = 0; i < p; ++i) {
        if (!std::isfinite(initial[i]) || &parameters[i] == &sumSquaredResiduals)
            return {Status::InvalidArgument, "invalid fit parameters or overlapping outputs"};
    }
    ResidualData data{callback, context, {}, {}, false};
    try { data.parameters.assign(initial, initial + p); data.residuals.resize(n); }
    catch (...) { return {Status::NumericalError, "fit buffer allocation failed"}; }
    gsl_multifit_nlinear_fdf functions{};
    functions.f = &invokeResidual; functions.n = n; functions.p = p; functions.params = &data;
    gsl_multifit_nlinear_parameters config = gsl_multifit_nlinear_default_parameters();
    std::unique_ptr<gsl_multifit_nlinear_workspace, decltype(&gsl_multifit_nlinear_free)> work(
        gsl_multifit_nlinear_alloc(gsl_multifit_nlinear_trust, &config, n, p), &gsl_multifit_nlinear_free);
    if (!work) return {Status::NumericalError, "nonlinear fit workspace allocation failed"};
    gsl_vector_view guess = gsl_vector_view_array(data.parameters.data(), p);
    if (gsl_multifit_nlinear_init(&guess.vector, &functions, work.get()) != GSL_SUCCESS || data.failed)
        return {Status::NumericalError, "nonlinear fit initialization failed"};
    int info = 0;
    if (gsl_multifit_nlinear_driver(maxIterations, tolerance, tolerance, tolerance, nullptr, nullptr, &info, work.get()) != GSL_SUCCESS || data.failed)
        return {Status::NumericalError, "nonlinear fit did not converge"};
    const gsl_vector* fitted = gsl_multifit_nlinear_position(work.get());
    const gsl_vector* residual = gsl_multifit_nlinear_residual(work.get());
    const double norm = gsl_blas_dnrm2(residual), sum = norm * norm;
    if (!std::isfinite(sum)) return {Status::NumericalError, "non-finite fit residual"};
    for (std::size_t i = 0; i < p; ++i) {
        data.parameters[i] = gsl_vector_get(fitted, i);
        if (!std::isfinite(data.parameters[i])) return {Status::NumericalError, "non-finite fit parameter"};
    }
    for (std::size_t i = 0; i < p; ++i) parameters[i] = data.parameters[i];
    sumSquaredResiduals = sum; return {Status::Ok, "ok"};
}
void registerFitting(Registry& r) {
    const Descriptor array{Type::Float64, Shape::Array};
    r.addWithoutArguments(Metadata{"linear_fit", "least squares and fitting", {array, array}, {array},
        "unweighted straight line; output c0,c1,cov00,cov01,cov11,sumsq", false,
        "per-instance object; no workspace", "invalid input, unsafe numeric range or GSL error"}, &makeLinearFit);
}
}
