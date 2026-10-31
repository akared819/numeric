// SPDX-License-Identifier: GPL-3.0-or-later
#include "numeric.h"
#include <gsl/gsl_rstat.h>
#include <gsl/gsl_errno.h>
#include <cmath>
#include <cstdint>
#include <memory>
#include <limits>

namespace numeric {
namespace {
struct WorkspaceDeleter {
    void operator()(gsl_rstat_workspace* p) const { if (p) gsl_rstat_free(p); }
};
class RunningMean : public Operation {
    std::unique_ptr<gsl_rstat_workspace, WorkspaceDeleter> workspace_;
    bool poisoned_;
public:
    explicit RunningMean(gsl_rstat_workspace* p) : workspace_(p), poisoned_(false) {}
    void reset() override {
        if (gsl_rstat_reset(workspace_.get()) == GSL_SUCCESS) poisoned_ = false;
        else poisoned_ = true;
    }
protected:
    Result executeImpl(const Input* in, std::size_t ni, Output* out, std::size_t no) override {
        if (out) for (std::size_t i = 0; i < no; ++i) out[i].count = 0;
        if (!in || !out || ni != 1 || no != 2)
            return {Status::InvalidArgument, "running_mean requires one input and two outputs"};
        if (in[0].type != Type::Float64 || in[0].shape != Shape::Scalar || !in[0].data ||
            in[0].count != 1 || in[0].capacity < 1 || in[0].stride != 1 ||
            out[0].type != Type::Float64 || out[1].type != Type::Float64 ||
            out[0].shape != Shape::Scalar || out[1].shape != Shape::Scalar ||
            !out[0].data || !out[1].data || out[0].capacity < 1 || out[1].capacity < 1 ||
            out[0].data == out[1].data || !std::isfinite(in[0].data[0]))
            return {Status::InvalidArgument, "invalid sample or output buffers"};
        if (poisoned_) return {Status::NumericalError, "reset required after numerical failure"};
        // A double can represent every integer only through 2^53.
        if (gsl_rstat_n(workspace_.get()) == std::numeric_limits<std::size_t>::max() ||
            static_cast<std::uint64_t>(gsl_rstat_n(workspace_.get())) >= UINT64_C(9007199254740992))
            return {Status::NumericalError, "sample count limit reached"};
        if (gsl_rstat_add(in[0].data[0], workspace_.get()) != GSL_SUCCESS) {
            poisoned_ = true; return {Status::NumericalError, "GSL update failed; reset required"};
        }
        const double value = gsl_rstat_mean(workspace_.get());
        if (!std::isfinite(value)) {
            poisoned_ = true; return {Status::NumericalError, "non-finite mean; reset required"};
        }
        out[0].data[0] = value;
        out[1].data[0] = static_cast<double>(gsl_rstat_n(workspace_.get()));
        out[0].count = out[1].count = 1;
        return {Status::Ok, "ok"};
    }
};
std::unique_ptr<Operation> makeRunningMean() {
    if (!isGslInitialized()) return std::unique_ptr<Operation>();
    std::unique_ptr<gsl_rstat_workspace, WorkspaceDeleter> workspace(gsl_rstat_alloc());
    if (!workspace) return std::unique_ptr<Operation>();
    std::unique_ptr<Operation> result(new RunningMean(workspace.get()));
    workspace.release();
    return result;
}
}
void registerRunningStatistics(Registry& r) {
    r.add(Metadata{"running_mean", "running statistics", {{Type::Float64, Shape::Scalar}},
                   {{Type::Float64, Shape::Scalar}, {Type::Float64, Shape::Scalar}},
                   "none", true, "per-instance GSL workspace; reset clears state",
                   "invalid sample or output; numerical error requires reset"}, &makeRunningMean);
}
}
