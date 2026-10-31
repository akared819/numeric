// SPDX-License-Identifier: GPL-3.0-or-later
#include "numericAckClient.h"
#include <cadef.h>
#include <epicsThread.h>
#include <epicsUnitTest.h>
#include <testMain.h>

static bool connectSnapshot(chid& snapshot) {
    return ca_context_create(ca_enable_preemptive_callback) == ECA_NORMAL &&
        ca_create_channel("numericNet:stats.VALL", nullptr, nullptr, CA_PRIORITY_DEFAULT, &snapshot) == ECA_NORMAL &&
        ca_pend_io(15.0) == ECA_NORMAL;
}
MAIN(numericNetworkTest) {
    testPlan(5);
    chid snapshot = nullptr;
    const bool connected = connectSnapshot(snapshot);
    testOk(connected, "separate IOC process is reachable over Channel Access");
    if (ca_current_context()) ca_context_destroy();
    if (!connected) { testSkip(4, "IOC unavailable"); return testDone(); }

    const numeric::Event first{81, 0, numeric::EventKind::Sample, 1};
    const auto timedOut = numeric::sendAcknowledged("numericNet:event", "numericNet:stats", first, 1, 1.0);
    testOk(timedOut.status == numeric::SendStatus::Uncertain, "client times out and disconnects during IOC processing");

    // The IOC must have processed the original request, rather than merely the retry.
    epicsThreadSleep(4.0);
    snapshot = nullptr;
    bool afterTimeout = connectSnapshot(snapshot);
    double response[11]{};
    afterTimeout = afterTimeout && ca_array_get(DBR_DOUBLE, 11, snapshot, response) == ECA_NORMAL &&
        ca_pend_io(10.0) == ECA_NORMAL && response[0] == 1 && response[6] == 1 &&
        response[7] == 81 && response[8] == 0 && response[9] == 0 && response[10] == 1;
    testOk(afterTimeout, "IOC completed the event after the sender lost its response");
    if (ca_current_context()) ca_context_destroy();

    const auto replay = numeric::sendAcknowledged("numericNet:event", "numericNet:stats", first, 1, 10.0);
    testOk(replay.status == numeric::SendStatus::Acknowledged && replay.statistics[0] == 1 && replay.statistics[6] == 1,
           "retry across a real CA connection does not double count");
    const numeric::Event second{81, 1, numeric::EventKind::Sample, 3};
    const auto next = numeric::sendAcknowledged("numericNet:event", "numericNet:stats", second, 1, 10.0);
    testOk(next.status == numeric::SendStatus::Acknowledged && next.statistics[0] == 2 && next.statistics[6] == 2,
           "next event is accepted after the retry");
    return testDone();
}
