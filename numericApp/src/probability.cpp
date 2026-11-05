// SPDX-License-Identifier: GPL-3.0-or-later
#include "numeric.h"
#include <gsl/gsl_rng.h>
#include <gsl/gsl_cdf.h>
#include <cmath>

namespace numeric {
UniformRandomStream::UniformRandomStream() : rng_(nullptr), seed_(0) {}
UniformRandomStream::~UniformRandomStream() { if (rng_) gsl_rng_free(static_cast<gsl_rng*>(rng_)); }
Result UniformRandomStream::initialize(std::uint32_t seed) {
    if (!isGslInitialized()) return {Status::NotInitialized, "GSL policy not initialized"};
    gsl_rng* fresh = gsl_rng_alloc(gsl_rng_mt19937);
    if (!fresh) return {Status::NumericalError, "random generator allocation failed"};
    gsl_rng_set(fresh, static_cast<unsigned long>(seed));
    if (rng_) gsl_rng_free(static_cast<gsl_rng*>(rng_));
    rng_ = fresh; seed_ = seed; return {Status::Ok, "ok"};
}
Result UniformRandomStream::reset() {
    if (!rng_) return {Status::NotInitialized, "random generator not initialized"};
    gsl_rng_set(static_cast<gsl_rng*>(rng_), static_cast<unsigned long>(seed_));
    return {Status::Ok, "ok"};
}
Result UniformRandomStream::fill(double* output, std::size_t count, std::size_t capacity) {
    if (!rng_) return {Status::NotInitialized, "random generator not initialized"};
    if (!output || !count || count > capacity) return {Status::InvalidArgument, "invalid random output buffer"};
    gsl_rng* rng = static_cast<gsl_rng*>(rng_);
    for (std::size_t i = 0; i < count; ++i) output[i] = gsl_rng_uniform(rng);
    return {Status::Ok, "ok"};
}
namespace {
class NormalCdf : public Operation {
public:
    void reset() override {}
protected:
    Result executeImpl(const Input* in, std::size_t ni, Output* out, std::size_t no) override {
        if (out) for (std::size_t i = 0; i < no; ++i) out[i].count = 0;
        if (!in || !out || ni != 1 || no != 1)
            return {Status::InvalidArgument, "normal_cdf requires one input and one output"};
        const Input& x = in[0]; Output& y = out[0];
        if (x.type != Type::Float64 || x.shape != Shape::Scalar || !x.data || x.count != 1 ||
            x.capacity < 1 || x.stride != 1 || y.type != Type::Float64 || y.shape != Shape::Scalar ||
            !y.data || y.capacity < 1 || !std::isfinite(x.data[0]))
            return {Status::InvalidArgument, "invalid normal_cdf input or output"};
        const double value = gsl_cdf_ugaussian_P(x.data[0]);
        if (!std::isfinite(value) || value < 0 || value > 1)
            return {Status::NumericalError, "invalid GSL normal CDF result"};
        y.data[0] = value; y.count = 1;
        return {Status::Ok, "ok"};
    }
};
std::unique_ptr<Operation> makeNormalCdf() { return std::unique_ptr<Operation>(new NormalCdf); }
}
void registerProbability(Registry& r) {
    r.addWithoutArguments(Metadata{"normal_cdf", "probability distributions", {{Type::Float64, Shape::Scalar}},
                   {{Type::Float64, Shape::Scalar}}, "standard normal lower-tail CDF; finite scalar input", false,
                   "per-instance object; no workspace or random state", "invalid input or non-finite/out-of-range result"}, &makeNormalCdf);
}
}
