// SPDX-License-Identifier: GPL-3.0-or-later
#include "numeric.h"
#include <gsl/gsl_errno.h>
#include <gsl/gsl_filter.h>
#include <gsl/gsl_movstat.h>
#include <gsl/gsl_vector.h>
#include <cmath>
#include <climits>
#include <cstdint>
#include <limits>
#include <memory>

namespace numeric {
namespace {
struct MovFree { void operator()(gsl_movstat_workspace* p) const { if (p) gsl_movstat_free(p); } };
struct FilterFree { void operator()(gsl_filter_gaussian_workspace* p) const { if (p) gsl_filter_gaussian_free(p); } };
bool scalar(const Input& x) {
    return x.type == Type::Float64 && x.shape == Shape::Scalar && x.data &&
           x.count == 1 && x.capacity >= 1 && x.stride == 1 && std::isfinite(x.data[0]);
}
class WindowOperation : public Operation {
    bool gaussian_;
    std::size_t window_ = 0;
    std::unique_ptr<gsl_movstat_workspace, MovFree> moving_;
    std::unique_ptr<gsl_filter_gaussian_workspace, FilterFree> filter_;
public:
    explicit WindowOperation(bool gaussian) : gaussian_(gaussian) {}
    void reset() override { moving_.reset(); filter_.reset(); window_ = 0; }
protected:
    Result executeImpl(const Input* in, std::size_t ni, Output* out, std::size_t no) override {
        if (out) for (std::size_t i = 0; i < no; ++i) out[i].count = 0;
        if (!in || !out || ni != (gaussian_ ? 3u : 2u) || no != 1)
            return {Status::InvalidArgument, "window operation has wrong input/output count"};
        const Input& x = in[0]; Output& y = out[0];
        if (x.type != Type::Float64 || x.shape != Shape::Array || !x.data || !x.count || x.count > INT_MAX ||
            x.stride != 1 || x.count > x.capacity || y.type != Type::Float64 ||
            y.shape != Shape::Array || !y.data || y.capacity < x.count || !scalar(in[1]) ||
            (gaussian_ && !scalar(in[2])))
            return {Status::InvalidArgument, "invalid input, output, or parameter"};
        const double k = in[1].data[0];
        if (k < 3 || k > 4095 || std::floor(k) != k || std::fmod(k, 2.0) != 1 || k > x.count)
            return {Status::InvalidArgument, "window must be odd, 3..4095, and no larger than input"};
        const double alpha = gaussian_ ? in[2].data[0] : 1.0;
        if (gaussian_ && (alpha < 0.01 || alpha > 10.0))
            return {Status::InvalidArgument, "Gaussian alpha must be between 0.01 and 10"};
        if (x.count > std::numeric_limits<std::uintptr_t>::max() / sizeof(double))
            return {Status::InvalidArgument, "buffer size overflow"};
        const std::uintptr_t bytes = x.count * sizeof(double);
        const std::uintptr_t a = reinterpret_cast<std::uintptr_t>(x.data), b = reinterpret_cast<std::uintptr_t>(y.data);
        if (a > std::numeric_limits<std::uintptr_t>::max() - bytes ||
            b > std::numeric_limits<std::uintptr_t>::max() - bytes ||
            (a != b && a < b + bytes && b < a + bytes))
            return {Status::InvalidArgument, "buffers partially overlap or overflow"};
        for (std::size_t i = 0; i < x.count; ++i)
            if (!std::isfinite(x.data[i])) return {Status::InvalidArgument, "input must be finite"};
        const std::size_t requested = static_cast<std::size_t>(k);
        if (window_ != requested) {
            if (gaussian_) {
                std::unique_ptr<gsl_filter_gaussian_workspace, FilterFree> fresh(gsl_filter_gaussian_alloc(requested));
                if (!fresh) return {Status::NumericalError, "Gaussian workspace allocation failed"};
                filter_.swap(fresh);
            } else {
                std::unique_ptr<gsl_movstat_workspace, MovFree> fresh(gsl_movstat_alloc(requested));
                if (!fresh) return {Status::NumericalError, "moving mean workspace allocation failed"};
                moving_.swap(fresh);
            }
            window_ = requested;
        }
        gsl_vector_const_view input = gsl_vector_const_view_array(x.data, x.count);
        gsl_vector_view output = gsl_vector_view_array(y.data, x.count);
        const int status = gaussian_ ? gsl_filter_gaussian(GSL_FILTER_END_PADVALUE, alpha, 0, &input.vector, &output.vector, filter_.get())
                                     : gsl_movstat_mean(GSL_MOVSTAT_END_PADVALUE, &input.vector, &output.vector, moving_.get());
        if (status != GSL_SUCCESS) return {Status::NumericalError, "GSL window operation failed"};
        for (std::size_t i = 0; i < x.count; ++i)
            if (!std::isfinite(y.data[i])) return {Status::NumericalError, "non-finite output"};
        y.count = x.count;
        return {Status::Ok, "ok"};
    }
};
std::unique_ptr<Operation> moving() { return std::unique_ptr<Operation>(new WindowOperation(false)); }
std::unique_ptr<Operation> gaussian() { return std::unique_ptr<Operation>(new WindowOperation(true)); }
}
void registerWindowOperations(Registry& r) {
    const Descriptor array{Type::Float64, Shape::Array}, scalarDesc{Type::Float64, Shape::Scalar};
    r.add(Metadata{"moving_mean", "moving statistics", {array, scalarDesc}, {array},
                   "odd centered window 3..4095; endpoint value padding", false,
                   "per-instance cached GSL workspace; reset frees it", "invalid buffers or parameters; allocation or numerical error"}, &moving);
    r.add(Metadata{"gaussian_smooth", "filter", {array, scalarDesc, scalarDesc}, {array},
                   "odd window 3..4095; alpha 0.01..10; endpoint value padding", false,
                   "per-instance cached GSL workspace; reset frees it", "invalid buffers or parameters; allocation or numerical error"}, &gaussian);
}
}
