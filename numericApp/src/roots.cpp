// SPDX-License-Identifier: GPL-3.0-or-later
#include "numeric.h"
#include <gsl/gsl_errno.h>
#include <gsl/gsl_poly.h>
#include <gsl/gsl_roots.h>
#include <cmath>
#include <memory>

namespace numeric {
namespace {
struct SolverFree { void operator()(gsl_root_fsolver* p) const { if (p) gsl_root_fsolver_free(p); } };
struct FunctionData { const double* coefficients; int count; bool failed; };
double polynomial(double x, void* ptr) {
    FunctionData* f=static_cast<FunctionData*>(ptr);
    if (!std::isfinite(x)) { f->failed=true; return 0; }
    const double value=gsl_poly_eval(f->coefficients,f->count,x);
    if (!std::isfinite(value)) { f->failed=true; return 0; }
    return value;
}
bool scalar(const Input& x) {
    return x.type==Type::Float64 && x.shape==Shape::Scalar && x.data && x.count==1 && x.capacity>=1 && x.stride==1 && std::isfinite(x.data[0]);
}
class PolynomialRoot : public Operation {
    std::unique_ptr<gsl_root_fsolver,SolverFree> solver_;
public:
    void reset() override { solver_.reset(); }
protected:
    Result executeImpl(const Input* in,std::size_t ni,Output* out,std::size_t no) override {
        if (out) for (std::size_t i=0;i<no;++i) out[i].count=0;
        if (!in || !out || ni!=3 || no!=1) return {Status::InvalidArgument,"polynomial root requires coefficients and a bracket"};
        const Input& c=in[0]; Output& result=out[0];
        if (c.type!=Type::Float64 || c.shape!=Shape::Array || !c.data || c.count<2 || c.count>64 || c.count>c.capacity || c.stride!=1 ||
            !scalar(in[1]) || !scalar(in[2]) || result.type!=Type::Float64 || result.shape!=Shape::Scalar || !result.data || !result.capacity)
            return {Status::InvalidArgument,"invalid root inputs or output"};
        for (std::size_t i=0;i<c.count;++i) if (!std::isfinite(c.data[i])) return {Status::InvalidArgument,"coefficients must be finite"};
        double lower=in[1].data[0], upper=in[2].data[0];
        if (!(lower<upper) || !std::isfinite(upper-lower)) return {Status::InvalidArgument,"invalid root bracket"};
        FunctionData data{c.data,static_cast<int>(c.count),false};
        double flo=polynomial(lower,&data), fhi=polynomial(upper,&data);
        if (data.failed) return {Status::NumericalError,"non-finite polynomial value"};
        if (flo==0 || fhi==0) { result.data[0]=flo==0 ? lower : upper; result.count=1; return {Status::Ok,"ok"}; }
        if (std::signbit(flo)==std::signbit(fhi)) return {Status::InvalidArgument,"bracket must contain a sign change"};
        if (!solver_) {
            solver_.reset(gsl_root_fsolver_alloc(gsl_root_fsolver_brent));
            if (!solver_) return {Status::NumericalError,"root solver allocation failed"};
        }
        gsl_function function{&polynomial,&data};
        if (gsl_root_fsolver_set(solver_.get(),&function,lower,upper)!=GSL_SUCCESS || data.failed)
            return {Status::NumericalError,"GSL root solver initialization failed"};
        for (int i=0;i<128;++i) {
            if (gsl_root_fsolver_iterate(solver_.get())!=GSL_SUCCESS || data.failed)
                return {Status::NumericalError,"GSL root iteration failed"};
            lower=gsl_root_fsolver_x_lower(solver_.get()); upper=gsl_root_fsolver_x_upper(solver_.get());
            const double root=gsl_root_fsolver_root(solver_.get());
            if (!std::isfinite(lower) || !std::isfinite(upper) || !std::isfinite(root))
                return {Status::NumericalError,"non-finite root iteration"};
            const int status=gsl_root_test_interval(lower,upper,1e-12,1e-12);
            if (status==GSL_SUCCESS) { result.data[0]=root; result.count=1; return {Status::Ok,"ok"}; }
            if (status!=GSL_CONTINUE) return {Status::NumericalError,"root convergence test failed"};
        }
        return {Status::NumericalError,"root solver did not converge within 128 iterations"};
    }
};
std::unique_ptr<Operation> makePolynomialRoot() { return std::unique_ptr<Operation>(new PolynomialRoot); }
}
void registerRoots(Registry& r) {
    const Descriptor array{Type::Float64,Shape::Array}, scalarDesc{Type::Float64,Shape::Scalar};
    r.add(Metadata{"polynomial_root", "roots", {array,scalarDesc,scalarDesc}, {scalarDesc},
        "bracketed polynomial root, Brent, abs/rel tolerance 1e-12, at most 128 iterations", false,
        "per-instance reusable GSL root solver; reset releases it", "invalid bracket, allocation, non-finite evaluation or nonconvergence"}, &makePolynomialRoot);
}
}
