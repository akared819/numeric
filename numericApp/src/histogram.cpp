// SPDX-License-Identifier: GPL-3.0-or-later
#include "numeric.h"
#include <gsl/gsl_errno.h>
#include <gsl/gsl_histogram.h>
#include <cmath>
#include <cstdint>
#include <memory>

namespace numeric {
namespace {
struct HistFree { void operator()(gsl_histogram* h) const { if (h) gsl_histogram_free(h); } };
bool scalar(const Input& x) {
    return x.type == Type::Float64 && x.shape == Shape::Scalar && x.data && x.count == 1 &&
           x.capacity >= 1 && x.stride == 1 && std::isfinite(x.data[0]);
}
class UniformHistogram : public Operation {
    std::unique_ptr<gsl_histogram, HistFree> histogram_;
    std::size_t bins_ = 0;
public:
    void reset() override { histogram_.reset(); bins_ = 0; }
protected:
    Result executeImpl(const Input* in, std::size_t ni, Output* out, std::size_t no) override {
        if (out) for (std::size_t i = 0; i < no; ++i) out[i].count = 0;
        if (!in || !out || ni != 4 || no != 1)
            return {Status::InvalidArgument, "histogram_uniform requires samples, lower, upper and bin count"};
        const Input& x = in[0]; Output& y = out[0];
        if (x.type != Type::Float64 || x.shape != Shape::Array || !x.data || !x.count ||
            x.stride != 1 || x.count > x.capacity || static_cast<std::uint64_t>(x.count) > UINT64_C(9007199254740992) ||
            !scalar(in[1]) || !scalar(in[2]) || !scalar(in[3]) ||
            y.type != Type::Float64 || y.shape != Shape::Array || !y.data)
            return {Status::InvalidArgument, "invalid histogram buffers or parameters"};
        const double lower = in[1].data[0], upper = in[2].data[0], requested = in[3].data[0];
        if (!(lower < upper) || !std::isfinite(upper - lower) || requested < 1 || requested > 65536 ||
            std::floor(requested) != requested || requested > y.capacity)
            return {Status::InvalidArgument, "invalid histogram range, bin count or output capacity"};
        const std::size_t bins = static_cast<std::size_t>(requested);
        for (std::size_t i = 0; i < x.count; ++i)
            if (!std::isfinite(x.data[i]) || x.data[i] < lower || x.data[i] >= upper)
                return {Status::InvalidArgument, "samples must be finite and within [lower, upper)"};
        if (bins != bins_) {
            std::unique_ptr<gsl_histogram, HistFree> fresh(gsl_histogram_alloc(bins));
            if (!fresh) return {Status::NumericalError, "histogram allocation failed"};
            histogram_.swap(fresh); bins_ = bins;
        }
        if (gsl_histogram_set_ranges_uniform(histogram_.get(), lower, upper) != GSL_SUCCESS)
            return {Status::NumericalError, "GSL histogram range setup failed"};
        for (std::size_t i = 0; i < bins; ++i) {
            double lo, hi;
            if (gsl_histogram_get_range(histogram_.get(), i, &lo, &hi) != GSL_SUCCESS ||
                !std::isfinite(lo) || !std::isfinite(hi) || !(lo < hi))
                return {Status::NumericalError, "histogram bin edges are not representable"};
        }
        for (std::size_t i = 0; i < x.count; ++i)
            if (gsl_histogram_increment(histogram_.get(), x.data[i]) != GSL_SUCCESS)
                return {Status::NumericalError, "GSL histogram increment failed"};
        for (std::size_t i = 0; i < bins; ++i) y.data[i] = gsl_histogram_get(histogram_.get(), i);
        y.count = bins;
        return {Status::Ok, "ok"};
    }
};
std::unique_ptr<Operation> makeHistogram() { return std::unique_ptr<Operation>(new UniformHistogram); }
}
void registerHistogram(Registry& r) {
    const Descriptor array{Type::Float64, Shape::Array}, scalarDesc{Type::Float64, Shape::Scalar};
    r.add(Metadata{"histogram_uniform", "histograms", {array, scalarDesc, scalarDesc, scalarDesc}, {array},
                   "finite samples; lower inclusive, upper exclusive; 1..65536 bins", false,
                   "per-instance reusable GSL histogram; reset releases it", "invalid input, allocation or GSL numerical error"}, &makeHistogram);
}
}
