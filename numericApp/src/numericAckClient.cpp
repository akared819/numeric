// SPDX-License-Identifier: GPL-3.0-or-later
#include "numericAckClient.h"
#include <cadef.h>
#include <epicsEvent.h>
#include <atomic>
#include <cmath>
#include <string>

namespace {
struct Pending {
    epicsEventId done = epicsEventCreate(epicsEventEmpty);
    std::atomic<int> status{ECA_TIMEOUT};
    ~Pending() { if (done) epicsEventDestroy(done); }
};
void complete(event_handler_args args) {
    Pending* pending = static_cast<Pending*>(args.usr);
    pending->status.store(args.status);
    epicsEventSignal(pending->done);
}
struct Context {
    chid channels[13]{};
    bool created = false;
    ~Context() {
        if (created) {
            for (chid channel : channels) if (channel) ca_clear_channel(channel);
            ca_context_destroy();
        }
    }
};
bool put(chid channel, chtype type, unsigned long count, const void* value, Pending& pending, double timeout) {
    return pending.done && ca_array_put_callback(type, count, channel, value, complete, &pending) == ECA_NORMAL &&
        ca_flush_io() == ECA_NORMAL && epicsEventWaitWithTimeout(pending.done, timeout) == epicsEventWaitOK &&
        pending.status.load() == ECA_NORMAL;
}
enum class Attempt { Accepted, Uncertain };
Attempt sendOnce(const std::string& eventPv, const std::string& record, const numeric::Event& event,
                 double timeout, std::array<double, 7>& statistics) {
    // Pending callbacks remain alive until the CA context has been destroyed.
    Pending payloadDone;
    Context context;
    if (!payloadDone.done || ca_context_create(ca_enable_preemptive_callback) != ECA_NORMAL)
        return Attempt::Uncertain;
    context.created = true;
    std::string names[13] = {eventPv, record + ".VAL", record + ".VALA", record + ".VALB",
        record + ".VALC", record + ".VALD", record + ".VALE", record + ".VALF", record + ".VALG",
        record + ".VALH", record + ".VALI", record + ".VALJ", record + ".VALK"};
    for (unsigned i = 0; i < 13; ++i)
        if (ca_create_channel(names[i].c_str(), nullptr, nullptr, CA_PRIORITY_DEFAULT, &context.channels[i]) != ECA_NORMAL)
            return Attempt::Uncertain;
    if (ca_pend_io(timeout) != ECA_NORMAL) return Attempt::Uncertain;
    const double payload[4] = {static_cast<double>(event.session), static_cast<double>(event.sequence),
        event.kind == numeric::EventKind::Sample ? 0.0 : 1.0, event.value};
    // The waveform's local forward link processes the aSub while its lock set is held.
    if (!put(context.channels[0], DBR_DOUBLE, 4, payload, payloadDone, timeout)) return Attempt::Uncertain;
    dbr_long_t status = -1;
    if (ca_get(DBR_LONG, context.channels[1], &status) != ECA_NORMAL || ca_pend_io(timeout) != ECA_NORMAL)
        return Attempt::Uncertain;
    // A late earlier event may have changed VAL after this event completed.
    if (status != 0) return Attempt::Uncertain;
    double output[11]{};
    for (unsigned i = 0; i < 11; ++i)
        if (ca_get(DBR_DOUBLE, context.channels[i + 2], &output[i]) != ECA_NORMAL) return Attempt::Uncertain;
    if (ca_pend_io(timeout) != ECA_NORMAL || output[7] != static_cast<double>(event.session) ||
        output[8] != static_cast<double>(event.sequence) || output[9] != payload[2] || output[10] != event.value)
        return Attempt::Uncertain;
    for (unsigned i = 0; i < 7; ++i) statistics[i] = output[i];
    return Attempt::Accepted;
}
}
namespace numeric {
SendResult sendAcknowledged(const std::string& eventPv, const std::string& record, const Event& event,
                            unsigned attempts, double timeout) {
    SendResult result{SendStatus::InvalidArgument, {}};
    const std::uint64_t limit = UINT64_C(9007199254740992);
    if (eventPv.empty() || record.empty() || !event.session || event.session > limit || event.sequence > limit ||
        !attempts || !std::isfinite(timeout) || timeout <= 0 || ca_current_context() ||
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
