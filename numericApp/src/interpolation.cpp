// SPDX-License-Identifier: GPL-3.0-or-later
#include "numeric.h"
#include <gsl/gsl_errno.h>
#include <gsl/gsl_interp.h>
#include <cmath>
#include <memory>

namespace numeric {
namespace {
struct InterpFree { void operator()(gsl_interp* p) const { if (p) gsl_interp_free(p); } };
struct AccelFree { void operator()(gsl_interp_accel* p) const { if (p) gsl_interp_accel_free(p); } };
class LinearInterpolation : public Operation {
    std::unique_ptr<gsl_interp, InterpFree> interp_;
    std::unique_ptr<gsl_interp_accel, AccelFree> accel_;
    std::size_t size_ = 0;
public:
    void reset() override { interp_.reset(); accel_.reset(); size_ = 0; }
protected:
    Result executeImpl(const Input* in, std::size_t ni, Output* out, std::size_t no) override {
        if (out) for (std::size_t i = 0; i < no; ++i) out[i].count = 0;
        if (!in || !out || ni != 3 || no != 1)
            return {Status::InvalidArgument, "interpolation requires x, y, and query"};
        const Input &x = in[0], &y = in[1], &q = in[2]; Output& result = out[0];
        if (x.type != Type::Float64 || x.shape != Shape::Array || !x.data || x.count < 2 || x.count > x.capacity || x.stride != 1 ||
            y.type != Type::Float64 || y.shape != Shape::Array || !y.data || y.count != x.count || y.count > y.capacity || y.stride != 1 ||
            q.type != Type::Float64 || q.shape != Shape::Scalar || !q.data || q.count != 1 || q.capacity < 1 || q.stride != 1 ||
            result.type != Type::Float64 || result.shape != Shape::Scalar || !result.data || !result.capacity || !std::isfinite(q.data[0]))
            return {Status::InvalidArgument, "invalid interpolation buffers or query"};
        for (std::size_t i = 0; i < x.count; ++i) {
            if (!std::isfinite(x.data[i]) || !std::isfinite(y.data[i]))
                return {Status::InvalidArgument, "interpolation points must be finite"};
            if (i && (!(x.data[i-1] < x.data[i]) || !std::isfinite(x.data[i]-x.data[i-1])))
                return {Status::InvalidArgument, "x points must be increasing with finite intervals"};
        }
        if (q.data[0] < x.data[0] || q.data[0] > x.data[x.count-1])
            return {Status::InvalidArgument, "query outside interpolation domain"};
        if (size_ != x.count) {
            std::unique_ptr<gsl_interp, InterpFree> fresh(gsl_interp_alloc(gsl_interp_linear, x.count));
            if (!fresh) return {Status::NumericalError, "interpolation allocation failed"};
            interp_.swap(fresh); size_ = x.count;
        }
        if (!accel_) {
            accel_.reset(gsl_interp_accel_alloc());
            if (!accel_) return {Status::NumericalError, "interpolation accelerator allocation failed"};
        }
        gsl_interp_accel_reset(accel_.get());
        if (gsl_interp_init(interp_.get(), x.data, y.data, x.count) != GSL_SUCCESS)
            return {Status::NumericalError, "GSL interpolation initialization failed"};
        double value;
        if (gsl_interp_eval_e(interp_.get(), x.data, y.data, q.data[0], accel_.get(), &value) != GSL_SUCCESS || !std::isfinite(value))
            return {Status::NumericalError, "GSL interpolation failed or returned non-finite result"};
        result.data[0] = value; result.count = 1;
        return {Status::Ok, "ok"};
    }
};
std::unique_ptr<Operation> makeLinearInterpolation() { return std::unique_ptr<Operation>(new LinearInterpolation); }
}
void registerInterpolation(Registry& r) {
    const Descriptor array{Type::Float64, Shape::Array}, scalar{Type::Float64, Shape::Scalar};
    r.add(Metadata{"interp_linear", "interpolation", {array, array, scalar}, {scalar},
        "strictly increasing finite x, finite y and query in closed x domain", false,
        "per-instance reusable GSL interpolation and accelerator; reset releases both", "invalid input, allocation or GSL numerical error"}, &makeLinearInterpolation);
}
}
