// SPDX-License-Identifier: GPL-3.0-or-later
#include "numeric.h"
#include <gsl/gsl_errno.h>
#include <gsl/gsl_rstat.h>
#include <cmath>
#include <cstdint>
#include <limits>
#include <memory>

namespace numeric {
namespace {
struct WorkspaceDeleter {
    void operator()(gsl_rstat_workspace* p) const { if (p) gsl_rstat_free(p); }
};
class RunningStats : public Operation {
    std::unique_ptr<gsl_rstat_workspace, WorkspaceDeleter> workspace_;
    bool poisoned_ = false;
public:
    explicit RunningStats(gsl_rstat_workspace* p) : workspace_(p) {}
    void reset() override { poisoned_ = gsl_rstat_reset(workspace_.get()) != GSL_SUCCESS; }
protected:
    Result executeImpl(const Input* in, std::size_t ni, Output* out, std::size_t no) override {
        if (out) for (std::size_t i = 0; i < no; ++i) out[i].count = 0;
        if (!in || !out || ni != 1 || no != 7)
            return {Status::InvalidArgument, "running_stats requires one input and seven outputs"};
        if (in[0].type != Type::Float64 || in[0].shape != Shape::Scalar || !in[0].data ||
            in[0].count != 1 || in[0].capacity < 1 || in[0].stride != 1 || !std::isfinite(*in[0].data))
            return {Status::InvalidArgument, "invalid sample"};
        for (std::size_t i = 0; i < 7; ++i) {
            if (out[i].type != Type::Float64 || out[i].shape != Shape::Scalar || !out[i].data || out[i].capacity < 1)
                return {Status::InvalidArgument, "invalid output"};
            for (std::size_t j = 0; j < i; ++j)
                if (out[i].data == out[j].data) return {Status::InvalidArgument, "outputs overlap"};
        }
        if (poisoned_) return {Status::NumericalError, "reset required after numerical failure"};
        const std::size_t oldCount = gsl_rstat_n(workspace_.get());
        if (oldCount == std::numeric_limits<std::size_t>::max() ||
            static_cast<std::uint64_t>(oldCount) >= UINT64_C(9007199254740992))
            return {Status::NumericalError, "sample count limit reached"};
        if (gsl_rstat_add(*in[0].data, workspace_.get()) != GSL_SUCCESS) {
            poisoned_ = true; return {Status::NumericalError, "GSL update failed; reset required"};
        }
        const std::size_t count = gsl_rstat_n(workspace_.get());
        const double nan = std::numeric_limits<double>::quiet_NaN();
        const double values[] = {gsl_rstat_mean(workspace_.get()),
                                 count > 1 ? gsl_rstat_variance(workspace_.get()) : nan,
                                 count > 1 ? gsl_rstat_sd(workspace_.get()) : nan,
                                 gsl_rstat_rms(workspace_.get()), gsl_rstat_min(workspace_.get()),
                                 gsl_rstat_max(workspace_.get()), static_cast<double>(count)};
        for (std::size_t i = 0; i < 7; ++i) {
            if (count == 1 && (i == 1 || i == 2)) continue;
            if (!std::isfinite(values[i])) {
                poisoned_ = true; return {Status::NumericalError, "non-finite statistic; reset required"};
            }
        }
        for (std::size_t i = 0; i < 7; ++i) { *out[i].data = values[i]; out[i].count = 1; }
        return {Status::Ok, "ok"};
    }
};
std::unique_ptr<Operation> makeRunningStats() {
    if (!isGslInitialized()) return std::unique_ptr<Operation>();
    std::unique_ptr<gsl_rstat_workspace, WorkspaceDeleter> workspace(gsl_rstat_alloc());
    if (!workspace) return std::unique_ptr<Operation>();
    std::unique_ptr<Operation> result(new RunningStats(workspace.get()));
    workspace.release();
    return result;
}
}
void registerRunningStats(Registry& r) {
    const Descriptor scalar{Type::Float64, Shape::Scalar};
    r.add(Metadata{"running_stats", "running statistics", {scalar},
                   {scalar, scalar, scalar, scalar, scalar, scalar, scalar},
                   "none; sample variance uses N-1", true,
                   "per-instance GSL workspace; reset clears state",
                   "invalid sample or output; numerical failure requires reset"}, &makeRunningStats);
}
}
