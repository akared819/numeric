// SPDX-License-Identifier: GPL-3.0-or-later
#include "numeric.h"
#include <gsl/gsl_integration.h>
#include <gsl/gsl_poly.h>
#include <cmath>
#include <memory>

namespace numeric {
namespace {
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
void registerIntegration(Registry& r) {
    const Descriptor array{Type::Float64, Shape::Array}, scalarDesc{Type::Float64, Shape::Scalar};
    r.add(Metadata{"polynomial_integral", "numerical integration", {array,scalarDesc,scalarDesc}, {scalarDesc},
        "32-point Gauss-Legendre integral of polynomial degree <=63", false,
        "per-instance reusable quadrature table; reset releases it", "invalid input, allocation or non-finite result"}, &makePolynomialIntegral);
}
}
