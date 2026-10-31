// SPDX-License-Identifier: GPL-3.0-or-later
#include <dbAccess.h>
#include <dbUnitTest.h>
#include <cadef.h>
#include <envDefs.h>
#include <epicsEvent.h>
#include <iocsh.h>
#include <testMain.h>

extern "C" int numericIocTest_registerRecordDeviceDriver(struct dbBase*);
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
    testPlan(8);
    epicsEnvSet("EPICS_CA_AUTO_ADDR_LIST", "NO");
    epicsEnvSet("EPICS_CA_ADDR_LIST", "127.0.0.1");
    epicsEnvSet("EPICS_CAS_INTF_ADDR_LIST", "127.0.0.1");
    testdbPrepare();
    testdbReadDatabase("dbd/numericIocTest.dbd", nullptr, nullptr);
    numericIocTest_registerRecordDeviceDriver(pdbbase);
    testdbReadDatabase("numericApp/src/numericIocTest.db", nullptr, nullptr);
    iocshCmd("numericInitialize");
    testIocInitOk();
    putResult.done = epicsEventCreate(epicsEventEmpty);
    if (!putResult.done || ca_context_create(ca_enable_preemptive_callback) != ECA_NORMAL) {
        testFail("CA context creation"); testSkip(7, "CA context unavailable");
        if (putResult.done) epicsEventDestroy(putResult.done);
        testIocShutdownOk(); testdbCleanup(); return testDone();
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
    if (connected) { ca_clear_channel(payload); ca_clear_channel(proc); ca_clear_channel(count); ca_clear_channel(session); ca_clear_channel(sequence); ca_clear_channel(status); }
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
    ca_context_destroy();
    epicsEventDestroy(putResult.done);
    testIocShutdownOk(); testdbCleanup(); return testDone();
}
