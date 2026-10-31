// SPDX-License-Identifier: GPL-3.0-or-later
#include <dbAccess.h>
#include <dbUnitTest.h>
#include <aSubRecord.h>
#include <cadef.h>
#include <epicsEvent.h>
#include <registryFunction.h>
#include <testMain.h>
#include <atomic>

extern "C" int numericBaseOnly_registerRecordDeviceDriver(struct dbBase*);
struct Result { epicsEventId done; std::atomic<int> status{ECA_TIMEOUT}; };
static void completed(event_handler_args args) {
    Result* result = static_cast<Result*>(args.usr);
    result->status.store(args.status);
    epicsEventSignal(result->done);
}
static long calculate(aSubRecord* record) {
    record->neva = 0;
    if (!record->a || record->nea != 4) return -1;
    const double* input = static_cast<const double*>(record->a);
    if (static_cast<unsigned>(input[1]) % 2) return -1;
    *static_cast<double*>(record->vala) = input[3];
    record->neva = 1;
    return 0;
}
MAIN(numericBaseNotifyTest) {
    testPlan(1);
    testdbPrepare();
    testdbReadDatabase("dbd/numericBaseOnly.dbd", nullptr, nullptr);
    numericBaseOnly_registerRecordDeviceDriver(pdbbase);
    registryFunctionAdd("baseNotifyCalculate", reinterpret_cast<REGISTRYFUNCTION>(calculate));
    testdbReadDatabase("numericApp/src/numericBaseNotifyTest.db", nullptr, nullptr);
    testIocInitOk();
    bool success = true;
    unsigned iteration = 0;
    for (; iteration < 500 && success; ++iteration) {
        Result result;
        result.done = epicsEventCreate(epicsEventEmpty);
        chid channel = nullptr;
        const bool context = ca_context_create(ca_enable_preemptive_callback) == ECA_NORMAL;
        const double event[4] = {1, static_cast<double>(iteration), 0, 1};
        success = result.done && context &&
            ca_create_channel("baseNotify:event", nullptr, nullptr, CA_PRIORITY_DEFAULT, &channel) == ECA_NORMAL &&
            ca_pend_io(5.0) == ECA_NORMAL &&
            ca_array_put_callback(DBR_DOUBLE, 4, channel, event, completed, &result) == ECA_NORMAL &&
            ca_flush_io() == ECA_NORMAL && epicsEventWaitWithTimeout(result.done, 5.0) == epicsEventWaitOK &&
            result.status.load() == ECA_NORMAL;
        if (channel) ca_clear_channel(channel);
        if (context) ca_context_destroy();
        if (result.done) epicsEventDestroy(result.done);
    }
    testOk(success, "500 base-only local CA put callbacks and context teardowns (%u completed)", iteration);
    testIocShutdownOk(); testdbCleanup(); return testDone();
}
