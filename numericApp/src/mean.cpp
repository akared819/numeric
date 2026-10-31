// SPDX-License-Identifier: GPL-3.0-or-later
#include "numeric.h"
#include <gsl/gsl_statistics_double.h>
#include <cmath>
#include <limits>
namespace numeric {
namespace {
class BatchStatistic : public Operation {
    int kind_;
public:
    explicit BatchStatistic(int kind) : kind_(kind) {}
    Result executeImpl(const Input* in, std::size_t ni, Output* out, std::size_t no) override {
        if (out && no == 1) out[0].count = 0;
        if (!in || !out || ni != 1 || no != 1) return {Status::InvalidArgument, "batch statistic requires one input and one output"};
        const Input& x = in[0]; Output& y = out[0];
        if (x.type != Type::Float64 || x.shape != Shape::Array || y.type != Type::Float64 || y.shape != Shape::Scalar ||
            !x.data || !y.data || x.count < (kind_ == 1 || kind_ == 2 ? 2u : 1u) || !x.stride || !y.capacity ||
            (x.count - 1) > (std::numeric_limits<std::size_t>::max() - 1) / x.stride ||
            (x.count - 1) * x.stride >= x.capacity)
            return {Status::InvalidArgument, "invalid buffer, type, count, stride or capacity"};
        for (std::size_t i = 0; i < x.count; ++i)
            if (!std::isfinite(x.data[i * x.stride])) return {Status::InvalidArgument, "input must be finite"};
        double value;
        if (kind_ == 0) value = gsl_stats_mean(x.data, x.stride, x.count);
        else if (kind_ == 1) value = gsl_stats_variance(x.data, x.stride, x.count);
        else if (kind_ == 2) value = gsl_stats_sd(x.data, x.stride, x.count);
        else {
            const double mean = gsl_stats_mean(x.data, x.stride, x.count);
            if (!std::isfinite(mean)) return {Status::NumericalError, "non-finite mean"};
            value = kind_ == 3 ? gsl_stats_variance_with_fixed_mean(x.data, x.stride, x.count, mean)
                               : gsl_stats_sd_with_fixed_mean(x.data, x.stride, x.count, mean);
        }
        if (!std::isfinite(value)) return {Status::NumericalError, "non-finite result"};
        y.data[0] = value; y.count = 1;
        return {Status::Ok, "ok"};
    }
    void reset() override {}
};
class BatchMinmax : public Operation {
public:
    void reset() override {}
protected:
    Result executeImpl(const Input* in, std::size_t ni, Output* out, std::size_t no) override {
        if (out) for (std::size_t i = 0; i < no; ++i) out[i].count = 0;
        if (!in || !out || ni != 1 || no != 2) return {Status::InvalidArgument, "minmax requires one input and two outputs"};
        const Input& x = in[0];
        if (x.type != Type::Float64 || x.shape != Shape::Array || !x.data || !x.count || !x.stride ||
            (x.count - 1) > (std::numeric_limits<std::size_t>::max() - 1) / x.stride ||
            (x.count - 1) * x.stride >= x.capacity)
            return {Status::InvalidArgument, "invalid input buffer, count, stride or capacity"};
        for (std::size_t i = 0; i < 2; ++i)
            if (out[i].type != Type::Float64 || out[i].shape != Shape::Scalar || !out[i].data || !out[i].capacity)
                return {Status::InvalidArgument, "invalid output"};
        if (out[0].data == out[1].data) return {Status::InvalidArgument, "outputs overlap"};
        for (std::size_t i = 0; i < x.count; ++i)
            if (!std::isfinite(x.data[i * x.stride])) return {Status::InvalidArgument, "input must be finite"};
        double minimum, maximum;
        gsl_stats_minmax(&minimum, &maximum, x.data, x.stride, x.count);
        *out[0].data = minimum; *out[1].data = maximum;
        out[0].count = out[1].count = 1;
        return {Status::Ok, "ok"};
    }
};
class BatchCovariance : public Operation {
    bool correlation_;
public:
    explicit BatchCovariance(bool correlation) : correlation_(correlation) {}
    void reset() override {}
protected:
    Result executeImpl(const Input* in, std::size_t ni, Output* out, std::size_t no) override {
        if (out) for (std::size_t i = 0; i < no; ++i) out[i].count = 0;
        if (!in || !out || ni != 2 || no != 1) return {Status::InvalidArgument, "covariance requires two inputs and one output"};
        if (out[0].type != Type::Float64 || out[0].shape != Shape::Scalar || !out[0].data || !out[0].capacity)
            return {Status::InvalidArgument, "invalid output"};
        if (in[0].count < 2 || in[0].count != in[1].count)
            return {Status::InvalidArgument, "inputs must contain the same number of samples, at least two"};
        for (std::size_t j = 0; j < 2; ++j) {
            const Input& x = in[j];
            if (x.type != Type::Float64 || x.shape != Shape::Array || !x.data || !x.stride ||
                (x.count - 1) > (std::numeric_limits<std::size_t>::max() - 1) / x.stride ||
                (x.count - 1) * x.stride >= x.capacity)
                return {Status::InvalidArgument, "invalid input buffer, stride or capacity"};
            bool varying = false;
            double minimum = x.data[0], maximum = x.data[0], largest = 0;
            for (std::size_t i = 0; i < x.count; ++i) {
                const double value = x.data[i * x.stride];
                if (!std::isfinite(value)) return {Status::InvalidArgument, "inputs must be finite"};
                if (correlation_) {
                    if (value != x.data[0]) varying = true;
                    if (value < minimum) minimum = value;
                    if (value > maximum) maximum = value;
                    if (std::fabs(value) > largest) largest = std::fabs(value);
                }
            }
            if (correlation_ && !varying) return {Status::InvalidArgument, "correlation requires non-constant inputs"};
            if (correlation_) {
                // Bound intermediate sums on platforms where GSL uses double-sized long double.
                const double high = std::sqrt(std::numeric_limits<double>::max() / static_cast<double>(x.count)) / 4;
                const double low = 4 * std::sqrt(std::numeric_limits<double>::min());
                if (largest > high || maximum - minimum < low)
                    return {Status::NumericalError, "correlation input range exceeds conservative floating-point limits"};
            }
        }
        const double value = correlation_ ? gsl_stats_correlation(in[0].data, in[0].stride, in[1].data, in[1].stride, in[0].count)
                                          : gsl_stats_covariance(in[0].data, in[0].stride, in[1].data, in[1].stride, in[0].count);
        if (!std::isfinite(value)) return {Status::NumericalError, "non-finite result"};
        *out[0].data = value; out[0].count = 1;
        return {Status::Ok, "ok"};
    }
};
std::unique_ptr<Operation> makeCovariance() { return std::unique_ptr<Operation>(new BatchCovariance(false)); }
std::unique_ptr<Operation> makeCorrelation() { return std::unique_ptr<Operation>(new BatchCovariance(true)); }
std::unique_ptr<Operation> makeMinmax() { return std::unique_ptr<Operation>(new BatchMinmax()); }
std::unique_ptr<Operation> makeMean() { return std::unique_ptr<Operation>(new BatchStatistic(0)); }
std::unique_ptr<Operation> makeVariance() { return std::unique_ptr<Operation>(new BatchStatistic(1)); }
std::unique_ptr<Operation> makeStddev() { return std::unique_ptr<Operation>(new BatchStatistic(2)); }
std::unique_ptr<Operation> makePopulationVariance() { return std::unique_ptr<Operation>(new BatchStatistic(3)); }
std::unique_ptr<Operation> makePopulationStddev() { return std::unique_ptr<Operation>(new BatchStatistic(4)); }
}
void registerRunningStatistics(Registry&);
void registerRunningStats(Registry&);
void registerFft(Registry&);
void registerWindowOperations(Registry&);
void registerBuiltins(Registry& r) {
    r.add(Metadata{"correlation", "statistics", {{Type::Float64, Shape::Array}, {Type::Float64, Shape::Array}},
                   {{Type::Float64, Shape::Scalar}}, "Pearson correlation; at least two paired non-constant samples", false,
                   "per-instance object; no workspace", "invalid or constant input; unsupported numeric range or non-finite result"}, &makeCorrelation);
    r.add(Metadata{"covariance", "statistics", {{Type::Float64, Shape::Array}, {Type::Float64, Shape::Array}},
                   {{Type::Float64, Shape::Scalar}}, "sample covariance (N-1); at least two paired samples", false,
                   "per-instance object; no workspace", "invalid input or non-finite result"}, &makeCovariance);
    r.add(Metadata{"minmax", "statistics", {{Type::Float64, Shape::Array}},
                   {{Type::Float64, Shape::Scalar}, {Type::Float64, Shape::Scalar}},
                   "minimum and maximum; at least one finite sample", false,
                   "per-instance object; no workspace", "invalid input or output"}, &makeMinmax);
    r.add(Metadata{"mean", "statistics", {{Type::Float64, Shape::Array}},
                   {{Type::Float64, Shape::Scalar}}, "none", false,
                   "per-instance object; no workspace", "invalid argument or non-finite result"}, &makeMean);
    r.add(Metadata{"variance", "statistics", {{Type::Float64, Shape::Array}},
                   {{Type::Float64, Shape::Scalar}}, "sample variance (N-1); at least two samples", false,
                   "per-instance object; no workspace", "invalid argument or non-finite result"}, &makeVariance);
    r.add(Metadata{"stddev", "statistics", {{Type::Float64, Shape::Array}},
                   {{Type::Float64, Shape::Scalar}}, "sample standard deviation (N-1); at least two samples", false,
                   "per-instance object; no workspace", "invalid argument or non-finite result"}, &makeStddev);
    r.add(Metadata{"population_variance", "statistics", {{Type::Float64, Shape::Array}},
                   {{Type::Float64, Shape::Scalar}}, "population variance (N); at least one sample", false,
                   "per-instance object; no workspace", "invalid argument or non-finite result"}, &makePopulationVariance);
    r.add(Metadata{"population_stddev", "statistics", {{Type::Float64, Shape::Array}},
                   {{Type::Float64, Shape::Scalar}}, "population standard deviation (N); at least one sample", false,
                   "per-instance object; no workspace", "invalid argument or non-finite result"}, &makePopulationStddev);
    registerRunningStatistics(r);
    registerRunningStats(r);
    registerFft(r);
    registerWindowOperations(r);
}
}
