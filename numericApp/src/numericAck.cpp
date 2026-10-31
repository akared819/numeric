// SPDX-License-Identifier: GPL-3.0-or-later
#include "numericAck.h"
#include <cmath>
#include <limits>
#include <utility>

namespace numeric {
namespace {
const std::uint64_t maximumToken = UINT64_C(9007199254740992);
std::unique_ptr<Operation> makeOperation() {
    Registry registry;
    registerBuiltins(registry);
    return registry.create("running_stats");
}
}
AcknowledgedStream::AcknowledgedStream(std::unique_ptr<Operation> operation)
    : operation_(std::move(operation)) {}
std::unique_ptr<AcknowledgedStream> AcknowledgedStream::create() {
    if (!isGslInitialized()) return {};
    auto operation = makeOperation();
    if (!operation) return {};
    return std::unique_ptr<AcknowledgedStream>(new AcknowledgedStream(std::move(operation)));
}
Result AcknowledgedStream::submit(const Event& event, Acknowledgment& acknowledgment) {
    if (!isGslInitialized()) return {Status::NotInitialized, "GSL policy not initialized"};
    if (!event.session || event.session > maximumToken || event.sequence > maximumToken ||
        (event.kind != EventKind::Sample && event.kind != EventKind::Reset) ||
        !std::isfinite(event.value) || (event.kind == EventKind::Reset && event.value != 0))
        return {Status::InvalidArgument, "invalid event"};
    if (hasLast_) {
        if (event.session != last_.event.session) return {Status::InvalidArgument, "session mismatch"};
        if (event.sequence == last_.event.sequence) {
            if (event.kind != last_.event.kind || event.value != last_.event.value)
                return {Status::InvalidArgument, "conflicting duplicate"};
            acknowledgment = last_; acknowledgment.replayed = true;
            return {Status::Ok, "replayed"};
        }
        if (last_.event.sequence == maximumToken || event.sequence != last_.event.sequence + 1)
            return {Status::InvalidArgument, "unexpected sequence"};
    } else if (event.sequence != 0) return {Status::InvalidArgument, "session must start at sequence zero"};
    Acknowledgment next{};
    next.event = event;
    if (event.kind == EventKind::Reset) {
        // Construct a replacement first so failed allocation leaves prior state intact.
        auto fresh = makeOperation();
        if (!fresh) return {Status::NumericalError, "reset allocation failed"};
        operation_ = std::move(fresh);
        next.statistics.fill(std::numeric_limits<double>::quiet_NaN());
        next.statistics[6] = 0;
    } else {
        Input input{&event.value, 1, 1, 1, Type::Float64, Shape::Scalar};
        Output outputs[7];
        for (std::size_t i = 0; i < 7; ++i)
            outputs[i] = {&next.statistics[i], 1, 0, Type::Float64, Shape::Scalar};
        const Result result = operation_->execute(&input, 1, outputs, 7);
        if (result.status != Status::Ok) return result;
    }
    next.replayed = false;
    last_ = next; hasLast_ = true; acknowledgment = next;
    return {Status::Ok, "accepted"};
}
}
