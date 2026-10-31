// SPDX-License-Identifier: GPL-3.0-or-later
#include <cadef.h>
#include <epicsEvent.h>
#include <epicsUnitTest.h>
#include <testMain.h>
#include "numericAckClient.h"
#include "numericAckParse.h"
#include <limits>

struct PutResult { epicsEventId done; int status; };
static PutResult putResult;
static void putComplete(event_handler_args args) {
    PutResult* result = static_cast<PutResult*>(args.usr);
    result->status = args.status;
    epicsEventSignal(result->done);
}
static bool putWait(chid channel, chtype type, unsigned long count, const void* value) {
    putResult.status = ECA_TIMEOUT;
    const int status = ca_array_put_callback(type, count, channel, value, putComplete, &putResult);
    return status == ECA_NORMAL && ca_flush_io() == ECA_NORMAL &&
           epicsEventWaitWithTimeout(putResult.done, 5.0) == epicsEventWaitOK && putResult.status == ECA_NORMAL;
}
static bool connect(const char* name, chid& channel) {
    return ca_create_channel(name, nullptr, nullptr, CA_PRIORITY_DEFAULT, &channel) == ECA_NORMAL &&
           ca_pend_io(5.0) == ECA_NORMAL;
}
static bool getDouble(chid channel, double& value) {
    return ca_get(DBR_DOUBLE, channel, &value) == ECA_NORMAL && ca_pend_io(5.0) == ECA_NORMAL;
}
MAIN(numericAckCaTest) {
    testPlan(28);
    putResult.done = epicsEventCreate(epicsEventEmpty);
    if (!putResult.done || ca_context_create(ca_enable_preemptive_callback) != ECA_NORMAL) {
        testFail("CA context creation"); testSkip(27, "CA context unavailable");
        if (putResult.done) epicsEventDestroy(putResult.done);
        return testDone();
    }
    chid payload = nullptr, proc = nullptr, count = nullptr, session = nullptr, sequence = nullptr, status = nullptr;
    bool connected = connect("ackEvent", payload) && connect("ackStats.PROC", proc) &&
        connect("ackStats.VALG", count) && connect("ackSessionOut", session) && connect("ackSequenceOut", sequence) && connect("ackStats.VAL", status);
    testOk(connected, "CA channels connected");
    double value = 0, sess = 0, seq = 0, event[] = {57, 0, 0, 1};
    dbr_long_t process = 1;
    bool first = connected && putWait(payload, DBR_DOUBLE, 4, event) && putWait(proc, DBR_LONG, 1, &process) &&
        getDouble(count, value) && getDouble(session, sess) && getDouble(sequence, seq) && value == 1 && sess == 57 && seq == 0;
    testOk(first, "CA sample and acknowledgment");
    if (payload) ca_clear_channel(payload);
    if (proc) ca_clear_channel(proc);
    if (count) ca_clear_channel(count);
    if (session) ca_clear_channel(session);
    if (sequence) ca_clear_channel(sequence);
    if (status) ca_clear_channel(status);
    payload = proc = count = session = sequence = status = nullptr;
    connected = first && connect("ackEvent", payload) && connect("ackStats.PROC", proc) &&
        connect("ackStats.VALG", count) && connect("ackSessionOut", session) && connect("ackSequenceOut", sequence) && connect("ackStats.VAL", status);
    testOk(connected, "CA client reconnects");
    bool replay = connected && putWait(payload, DBR_DOUBLE, 4, event) && putWait(proc, DBR_LONG, 1, &process) &&
        getDouble(count, value) && getDouble(sequence, seq) && value == 1 && seq == 0;
    testOk(replay, "retry after client reconnect is idempotent");
    event[3] = 9;
    bool conflict = replay && putWait(payload, DBR_DOUBLE, 4, event) && putWait(proc, DBR_LONG, 1, &process) &&
        getDouble(status, value) && getDouble(sequence, seq) && value == -1 && seq == 0;
    testOk(conflict, "conflicting duplicate rejected without changing acknowledgment");
    event[1] = 2; event[3] = 3;
    bool gap = conflict && putWait(payload, DBR_DOUBLE, 4, event) && putWait(proc, DBR_LONG, 1, &process) &&
        getDouble(status, value) && getDouble(sequence, seq) && value == -1 && seq == 0;
    testOk(gap, "sequence gap rejected without changing acknowledgment");
    event[1] = 1; event[3] = 3;
    bool second = gap && putWait(payload, DBR_DOUBLE, 4, event) && putWait(proc, DBR_LONG, 1, &process) &&
        getDouble(count, value) && getDouble(sequence, seq) && value == 2 && seq == 1;
    testOk(second, "next acknowledged event advances once");
    event[1] = 2; event[2] = 1; event[3] = 0;
    bool reset = second && putWait(payload, DBR_DOUBLE, 4, event) && putWait(proc, DBR_LONG, 1, &process) &&
        getDouble(count, value) && getDouble(sequence, seq) && value == 0 && seq == 2;
    testOk(reset, "ordered reset is acknowledged");
    chid example = nullptr, exampleStatus = nullptr;
    const bool exampleConnected = connect("demo:event", example) && connect("demo:acknowledgedStats.VAL", exampleStatus);
    testOk(exampleConnected, "example database channels connect");
    const double oversized[] = {70, 0, 0, 9, 123};
    testOk(exampleConnected && putWait(example, DBR_DOUBLE, 5, oversized) &&
           getDouble(exampleStatus, value) && value == -1, "example rejects an oversized event without truncating it to four elements");
    if (payload) ca_clear_channel(payload);
    if (proc) ca_clear_channel(proc);
    if (count) ca_clear_channel(count);
    if (session) ca_clear_channel(session);
    if (sequence) ca_clear_channel(sequence);
    if (status) ca_clear_channel(status);
    if (example) ca_clear_channel(example);
    if (exampleStatus) ca_clear_channel(exampleStatus);
    ca_context_destroy();
    epicsEventDestroy(putResult.done);
    const numeric::Event exampleEvent{70, 0, numeric::EventKind::Sample, 9};
    const auto exampleSent = numeric::sendAcknowledged("demo:event", "demo:acknowledgedStats", exampleEvent, 1, 5.0);
    testOk(exampleSent.status == numeric::SendStatus::Acknowledged && exampleSent.statistics[6] == 1,
           "example accepts the first valid event after rejecting the oversized packet");
    numeric::Event clientEvent{61, 0, numeric::EventKind::Sample, 9};
    auto sent = numeric::sendAcknowledged("ackSafeEvent", "ackSafeStats", clientEvent, 2, 5.0);
    testOk(sent.status == numeric::SendStatus::Acknowledged && sent.statistics[0] == 9 && sent.statistics[6] == 1,
           "sender confirms sample and statistics");
    sent = numeric::sendAcknowledged("ackSafeEvent", "ackSafeStats", clientEvent, 2, 5.0);
    testOk(sent.status == numeric::SendStatus::Acknowledged && sent.statistics[6] == 1, "sender replay does not double count");
    clientEvent.value = 10;
    sent = numeric::sendAcknowledged("ackSafeEvent", "ackSafeStats", clientEvent, 1, 0.25);
    testOk(sent.status == numeric::SendStatus::Uncertain, "sender does not acknowledge a conflicting duplicate");
    clientEvent.value = 9;
    clientEvent.sequence = 2;
    sent = numeric::sendAcknowledged("ackSafeEvent", "ackSafeStats", clientEvent, 1, 0.25);
    testOk(sent.status == numeric::SendStatus::Uncertain, "sender reports sequence gap as uncertain");
    clientEvent.sequence = 1; clientEvent.kind = numeric::EventKind::Reset; clientEvent.value = 0;
    sent = numeric::sendAcknowledged("ackSafeEvent", "ackSafeStats", clientEvent, 2, 5.0);
    testOk(sent.status == numeric::SendStatus::Acknowledged && sent.statistics[6] == 0, "sender confirms reset");
    clientEvent.session = 0;
    sent = numeric::sendAcknowledged("ackSafeEvent", "ackSafeStats", clientEvent, 2, 5.0);
    testOk(sent.status == numeric::SendStatus::InvalidArgument, "sender rejects invalid session locally");
    clientEvent.session = 61;
    sent = numeric::sendAcknowledged("numeric:missing:event", "numeric:missing:record", clientEvent, 1, 0.2);
    testOk(sent.status == numeric::SendStatus::Uncertain, "sender reports unavailable IOC as uncertain");
    // Deliver an old request after a newer event has already been acknowledged.
    putResult.done = epicsEventCreate(epicsEventEmpty);
    bool lateConnected = putResult.done && ca_context_create(ca_enable_preemptive_callback) == ECA_NORMAL;
    chid late = nullptr, lateStatus = nullptr, snapshotCount = nullptr;
    lateConnected = lateConnected && connect("ackSafeEvent", late) && connect("ackSafeStats.VAL", lateStatus) &&
        connect("ackSafeStats.NEVL", snapshotCount);
    double oldEvent[] = {61, 0, 0, 9};
    bool lateWritten = lateConnected && putWait(late, DBR_DOUBLE, 4, oldEvent);
    testOk(lateWritten, "delayed old waveform write completes");
    testOk(lateWritten && getDouble(lateStatus, value) && value == -1, "late old event is rejected");
    testOk(lateWritten && getDouble(snapshotCount, value) && value == 0, "rejected event clears snapshot count");
    if (late) ca_clear_channel(late);
    if (lateStatus) ca_clear_channel(lateStatus);
    if (snapshotCount) ca_clear_channel(snapshotCount);
    if (ca_current_context()) ca_context_destroy();
    if (putResult.done) epicsEventDestroy(putResult.done);
    sent = numeric::sendAcknowledged("ackNoForward", "ackSafeStats", clientEvent, 1, 0.25);
    testOk(sent.status == numeric::SendStatus::Uncertain, "sender rejects an empty snapshot after failed processing");
    clientEvent.sequence = 2; clientEvent.kind = numeric::EventKind::Sample; clientEvent.value = 7;
    sent = numeric::sendAcknowledged("ackSafeEvent", "ackSafeStats", clientEvent, 2, 5.0);
    testOk(sent.status == numeric::SendStatus::Acknowledged && sent.statistics[0] == 7 && sent.statistics[6] == 1,
           "next event succeeds after delayed old write");
    bool snapshotConnected = ca_context_create(ca_enable_preemptive_callback) == ECA_NORMAL;
    chid snapshotChannel = nullptr;
    snapshotConnected = snapshotConnected && connect("ackSafeStats.VALL", snapshotChannel);
    double snapshot[11]{};
    bool snapshotRead = snapshotConnected && ca_array_get(DBR_DOUBLE, 11, snapshotChannel, snapshot) == ECA_NORMAL &&
        ca_pend_io(5.0) == ECA_NORMAL;
    testOk(snapshotRead, "complete snapshot is readable over Channel Access");
    testOk(snapshotRead && snapshot[0] == 7 && snapshot[6] == 1 && snapshot[7] == 61 && snapshot[8] == 2 &&
           snapshot[9] == 0 && snapshot[10] == 7, "snapshot combines statistics and matching event identity");
    if (snapshotChannel) ca_clear_channel(snapshotChannel);
    if (ca_current_context()) ca_context_destroy();
    double parsed = 0;
    testOk(numericParseSample("4.9406564584124654e-324", parsed) && parsed == std::numeric_limits<double>::denorm_min(),
           "sample parser accepts smallest positive subnormal");
    testOk(!numericParseSample("1e9999", parsed) && !numericParseSample("1e-9999", parsed),
           "sample parser rejects overflow and underflow to zero");
    testOk(!numericParseSample("nan", parsed) && !numericParseSample("1x", parsed), "sample parser rejects invalid input");
    return testDone();
}
