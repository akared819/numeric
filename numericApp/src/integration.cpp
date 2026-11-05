// SPDX-License-Identifier: GPL-3.0-or-later
#include "numeric.h"
#include <gsl/gsl_integration.h>
#include <gsl/gsl_poly.h>
#include <gsl/gsl_deriv.h>
#include <gsl/gsl_errno.h>
#include <gsl/gsl_roots.h>
#include <algorithm>
#include <cmath>
#include <limits>
#include <memory>

namespace numeric {
namespace {
struct CallbackData { ScalarCallback callback; void* context; bool failed; };
double invokeCallback(double x, void* raw) {
    CallbackData& data = *static_cast<CallbackData*>(raw);
    if (!std::isfinite(x)) { data.failed = true; return std::numeric_limits<double>::quiet_NaN(); }
    try {
        const double value = data.callback(x, data.context);
        if (std::isfinite(value)) return value;
    } catch (...) {}
    data.failed = true; return std::numeric_limits<double>::quiet_NaN();
}
struct TableFree { void operator()(gsl_integration_glfixed_table* p) const { if (p) gsl_integration_glfixed_table_free(p); } };
struct FunctionData { const double* coefficients; int count; bool failed; };
double polynomial(double x, void* data) {
    FunctionData* f = static_cast<FunctionData*>(data);
    if (!std::isfinite(x)) { f->failed = true; return 0; }
    const double value = gsl_poly_eval(f->coefficients, f->count, x);
    if (!std::isfinite(value)) { f->failed = true; return 0; }
    return value;
}
bool scalar(const Input& x) {
    return x.type == Type::Float64 && x.shape == Shape::Scalar && x.data && x.count == 1 && x.capacity >= 1 && x.stride == 1 && std::isfinite(x.data[0]);
}
class PolynomialIntegral : public Operation {
    std::unique_ptr<gsl_integration_glfixed_table, TableFree> table_;
public:
    void reset() override { table_.reset(); }
protected:
    Result executeImpl(const Input* in, std::size_t ni, Output* out, std::size_t no) override {
        if (out) for (std::size_t i = 0; i < no; ++i) out[i].count = 0;
        if (!in || !out || ni != 3 || no != 1) return {Status::InvalidArgument, "polynomial integration requires coefficients and two bounds"};
        const Input& c=in[0]; Output& result=out[0];
        if (c.type != Type::Float64 || c.shape != Shape::Array || !c.data || !c.count || c.count > 64 || c.count > c.capacity || c.stride != 1 ||
            !scalar(in[1]) || !scalar(in[2]) || result.type != Type::Float64 || result.shape != Shape::Scalar || !result.data || !result.capacity)
            return {Status::InvalidArgument, "invalid polynomial integral inputs or output"};
        for (std::size_t i=0; i<c.count; ++i) if (!std::isfinite(c.data[i])) return {Status::InvalidArgument, "coefficients must be finite"};
        const double a=in[1].data[0], b=in[2].data[0];
        if (!std::isfinite(a+b) || !std::isfinite(b-a)) return {Status::InvalidArgument, "integration bounds exceed safe range"};
        if (!table_) {
            table_.reset(gsl_integration_glfixed_table_alloc(32));
            if (!table_) return {Status::NumericalError, "integration table allocation failed"};
        }
        FunctionData data{c.data, static_cast<int>(c.count), false};
        gsl_function function{&polynomial, &data};
        const double value=gsl_integration_glfixed(&function, a, b, table_.get());
        if (data.failed || !std::isfinite(value)) return {Status::NumericalError, "non-finite integrand or integral"};
        result.data[0]=value; result.count=1; return {Status::Ok, "ok"};
    }
};
std::unique_ptr<Operation> makePolynomialIntegral() { return std::unique_ptr<Operation>(new PolynomialIntegral); }
}
Result differentiateCentral(ScalarCallback callback, void* context, double x, double step, double& value, double& absoluteError) {
    if (!isGslInitialized()) return {Status::NotInitialized, "GSL policy not initialized"};
    if (!callback || &value == &absoluteError || !std::isfinite(x) || !std::isfinite(step) || step <= 0 ||
        !std::isfinite(x - step) || !std::isfinite(x + step) || x - step == x || x + step == x)
        return {Status::InvalidArgument, "invalid differentiation inputs"};
    CallbackData data{callback, context, false}; gsl_function function{&invokeCallback, &data};
    double result = 0, error = 0;
    const int status = gsl_deriv_central(&function, x, step, &result, &error);
    if (data.failed || status != GSL_SUCCESS || !std::isfinite(result) || !std::isfinite(error) || error < 0)
        return {Status::NumericalError, "differentiation or callback failed"};
    value = result; absoluteError = error; return {Status::Ok, "ok"};
}
Result integrateAdaptive(ScalarCallback callback, void* context, double lower, double upper,
                         double absTol, double relTol, std::size_t limit, double& value, double& absoluteError) {
    if (!isGslInitialized()) return {Status::NotInitialized, "GSL policy not initialized"};
    if (!callback || &value == &absoluteError || !std::isfinite(lower) || !std::isfinite(upper) || lower >= upper ||
        !std::isfinite(upper - lower) || !std::isfinite(absTol) || !std::isfinite(relTol) || absTol < 0 || relTol < 0 ||
        (absTol == 0 && relTol < 50 * std::numeric_limits<double>::epsilon()) || limit == 0 || limit > 1000000)
        return {Status::InvalidArgument, "invalid integration inputs"};
    std::unique_ptr<gsl_integration_workspace, decltype(&gsl_integration_workspace_free)> workspace(gsl_integration_workspace_alloc(limit), &gsl_integration_workspace_free);
    if (!workspace) return {Status::NumericalError, "integration workspace allocation failed"};
    CallbackData data{callback, context, false}; gsl_function function{&invokeCallback, &data};
    double result = 0, error = 0;
    const int status = gsl_integration_qags(&function, lower, upper, absTol, relTol, limit, workspace.get(), &result, &error);
    if (data.failed || status != GSL_SUCCESS || !std::isfinite(result) || !std::isfinite(error) || error < 0)
        return {Status::NumericalError, "integration did not converge or callback failed"};
    value = result; absoluteError = error; return {Status::Ok, "ok"};
}
Result findRootBrent(ScalarCallback callback, void* context, double lower, double upper,
                     double absTol, double relTol, std::size_t maxIterations, double& root, double& bracketError) {
    if (!isGslInitialized()) return {Status::NotInitialized, "GSL policy not initialized"};
    if (!callback || &root == &bracketError || !std::isfinite(lower) || !std::isfinite(upper) || lower >= upper ||
        !std::isfinite(upper-lower) || !std::isfinite(absTol) || !std::isfinite(relTol) || absTol < 0 || relTol < 0 ||
        (absTol == 0 && relTol == 0) || !maxIterations || maxIterations > 1000000)
        return {Status::InvalidArgument, "invalid root-finding inputs"};
    CallbackData data{callback, context, false};
    const double fLower = invokeCallback(lower, &data), fUpper = invokeCallback(upper, &data);
    if (data.failed) return {Status::NumericalError, "root callback failed"};
    if (fLower == 0) { root = lower; bracketError = 0; return {Status::Ok, "ok"}; }
    if (fUpper == 0) { root = upper; bracketError = 0; return {Status::Ok, "ok"}; }
    if (std::signbit(fLower) == std::signbit(fUpper)) return {Status::InvalidArgument, "root is not bracketed"};
    std::unique_ptr<gsl_root_fsolver, decltype(&gsl_root_fsolver_free)> solver(gsl_root_fsolver_alloc(gsl_root_fsolver_brent), &gsl_root_fsolver_free);
    if (!solver) return {Status::NumericalError, "root solver allocation failed"};
    gsl_function function{&invokeCallback, &data};
    if (gsl_root_fsolver_set(solver.get(), &function, lower, upper) != GSL_SUCCESS || data.failed)
        return {Status::NumericalError, "root solver initialization failed"};
    for (std::size_t i = 0; i < maxIterations; ++i) {
        if (gsl_root_fsolver_iterate(solver.get()) != GSL_SUCCESS || data.failed) return {Status::NumericalError, "root solver iteration failed"};
        const double a = gsl_root_fsolver_x_lower(solver.get()), b = gsl_root_fsolver_x_upper(solver.get());
        const double candidate = gsl_root_fsolver_root(solver.get());
        if (!std::isfinite(a) || !std::isfinite(b) || !std::isfinite(candidate)) return {Status::NumericalError, "non-finite root result"};
        const int status = gsl_root_test_interval(a, b, absTol, relTol);
        if (status == GSL_SUCCESS) { root = candidate; bracketError = std::max(candidate-a, b-candidate); return {Status::Ok, "ok"}; }
        if (status != GSL_CONTINUE) return {Status::NumericalError, "root convergence test failed"};
    }
    return {Status::NumericalError, "root solver did not converge"};
}
void registerIntegration(Registry& r) {
    const Descriptor array{Type::Float64, Shape::Array}, scalarDesc{Type::Float64, Shape::Scalar};
    r.addWithoutArguments(Metadata{"polynomial_integral", "numerical integration", {array,scalarDesc,scalarDesc}, {scalarDesc},
        "32-point Gauss-Legendre integral of polynomial degree <=63", false,
        "per-instance reusable quadrature table; reset releases it", "invalid input, allocation or non-finite result"}, &makePolynomialIntegral);
}
}
