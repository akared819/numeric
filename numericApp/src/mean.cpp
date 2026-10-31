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
std::unique_ptr<Operation> makeMean() { return std::unique_ptr<Operation>(new BatchStatistic(0)); }
std::unique_ptr<Operation> makeVariance() { return std::unique_ptr<Operation>(new BatchStatistic(1)); }
std::unique_ptr<Operation> makeStddev() { return std::unique_ptr<Operation>(new BatchStatistic(2)); }
std::unique_ptr<Operation> makePopulationVariance() { return std::unique_ptr<Operation>(new BatchStatistic(3)); }
std::unique_ptr<Operation> makePopulationStddev() { return std::unique_ptr<Operation>(new BatchStatistic(4)); }
}
void registerRunningStatistics(Registry&);
void registerRunningStats(Registry&);
void registerBuiltins(Registry& r) {
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
}
}
