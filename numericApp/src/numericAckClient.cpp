// SPDX-License-Identifier: GPL-3.0-or-later
#include "numericAckClient.h"
#include <cadef.h>
#include <epicsEvent.h>
#include <atomic>
#include <chrono>
#include <cmath>
#include <string>
#include <thread>

namespace {
struct Pending {
    epicsEventId done = epicsEventCreate(epicsEventEmpty);
    std::atomic<int> status{ECA_TIMEOUT};
    ~Pending() { if (done) epicsEventDestroy(done); }
};
struct Snapshot {
    Pending pending;
    double values[11]{};
};
void getComplete(event_handler_args args) {
    Snapshot* snapshot = static_cast<Snapshot*>(args.usr);
    int status = args.status;
    if (status == ECA_NORMAL && args.count == 11 && args.dbr) {
        const dbr_double_t* values = static_cast<const dbr_double_t*>(args.dbr);
        for (unsigned i = 0; i < 11; ++i) snapshot->values[i] = values[i];
    } else {
        status = ECA_GETFAIL;
    }
    snapshot->pending.status.store(status);
    epicsEventSignal(snapshot->pending.done);
}
struct Context {
    chid channels[2]{};
    bool created = false;
    ~Context() {
        if (created) {
            for (chid channel : channels) if (channel) ca_clear_channel(channel);
            ca_context_destroy();
        }
    }
};
enum class Attempt { Accepted, Uncertain };
Attempt sendOnce(const std::string& eventPv, const std::string& record, const numeric::Event& event,
                 double timeout, std::array<double, 7>& statistics) {
    // Pending callbacks remain alive until the CA context has been destroyed.
    Snapshot snapshot;
    Context context;
    if (!snapshot.pending.done || ca_context_create(ca_enable_preemptive_callback) != ECA_NORMAL)
        return Attempt::Uncertain;
    context.created = true;
    std::string names[2] = {eventPv, record + ".VALL"};
    for (unsigned i = 0; i < 2; ++i)
        if (ca_create_channel(names[i].c_str(), nullptr, nullptr, CA_PRIORITY_DEFAULT, &context.channels[i]) != ECA_NORMAL)
            return Attempt::Uncertain;
    if (ca_pend_io(timeout) != ECA_NORMAL) return Attempt::Uncertain;
    const double payload[4] = {static_cast<double>(event.session), static_cast<double>(event.sequence),
        event.kind == numeric::EventKind::Sample ? 0.0 : 1.0, event.value};
    // The snapshot, not the write request, establishes processing completion.
    // An unacknowledged or dropped write is safe to retry with the same event.
    if (ca_element_count(context.channels[1]) < 11 ||
        ca_array_put(DBR_DOUBLE, 4, context.channels[0], payload) != ECA_NORMAL || ca_flush_io() != ECA_NORMAL)
        return Attempt::Uncertain;
    using Clock = std::chrono::steady_clock;
    const Clock::time_point deadline = Clock::now() +
        std::chrono::duration_cast<Clock::duration>(std::chrono::duration<double>(timeout));
    while (Clock::now() < deadline) {
        snapshot.pending.status.store(ECA_TIMEOUT);
        if (ca_array_get_callback(DBR_DOUBLE, 0, context.channels[1], getComplete, &snapshot) != ECA_NORMAL ||
            ca_flush_io() != ECA_NORMAL) return Attempt::Uncertain;
        const double remaining = std::chrono::duration<double>(deadline - Clock::now()).count();
        if (remaining <= 0 || epicsEventWaitWithTimeout(snapshot.pending.done, remaining) != epicsEventWaitOK)
            return Attempt::Uncertain;
        const double* output = snapshot.values;
        if (snapshot.pending.status.load() == ECA_NORMAL && output[7] == static_cast<double>(event.session) &&
            output[8] == static_cast<double>(event.sequence) && output[9] == payload[2] && output[10] == event.value) {
            for (unsigned i = 0; i < 7; ++i) statistics[i] = output[i];
            return Attempt::Accepted;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    return Attempt::Uncertain;
}
}
namespace numeric {
SendResult sendAcknowledged(const std::string& eventPv, const std::string& record, const Event& event,
                            unsigned attempts, double timeout) {
    SendResult result{SendStatus::InvalidArgument, {}};
    const std::uint64_t limit = UINT64_C(9007199254740992);
    if (eventPv.empty() || record.empty() || !event.session || event.session > limit || event.sequence > limit ||
        !attempts || !std::isfinite(timeout) || timeout <= 0 || timeout > 86400 || ca_current_context() ||
        (event.kind != EventKind::Sample && event.kind != EventKind::Reset) || !std::isfinite(event.value) ||
        (event.kind == EventKind::Reset && event.value != 0)) return result;
    for (unsigned i = 0; i < attempts; ++i) {
        const Attempt attempt = sendOnce(eventPv, record, event, timeout, result.statistics);
        if (attempt == Attempt::Accepted) { result.status = SendStatus::Acknowledged; return result; }
    }
    result.status = SendStatus::Uncertain;
    return result;
}
}
