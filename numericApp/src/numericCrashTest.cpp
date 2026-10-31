// SPDX-License-Identifier: GPL-3.0-or-later
#include "numericAckClient.h"
#include <epicsUnitTest.h>
#include <cstdio>
#include <cstring>

int main(int argc, char** argv) {
    if (argc != 2 || (std::strcmp(argv[1], "inflight") && std::strcmp(argv[1], "recovery"))) {
        std::fprintf(stderr, "Usage: numericCrashTest inflight|recovery\n");
        return 2;
    }
    if (!std::strcmp(argv[1], "inflight")) {
        testPlan(1);
        // The preceding network test accepted sequence 1 in session 81.
        const numeric::Event event{81, 2, numeric::EventKind::Sample, 42};
        const auto result = numeric::sendAcknowledged("numericNet:event", "numericNet:stats", event, 1, 10.0);
        testOk(result.status == numeric::SendStatus::Uncertain,
               "sender reports uncertainty when IOC dies during event processing");
    } else {
        testPlan(2);
        const numeric::Event stale{202, 1, numeric::EventKind::Sample, 10};
        const auto old = numeric::sendAcknowledged("restart:event", "restart:acknowledgedStats", stale, 1, 1.0);
        testOk(old.status == numeric::SendStatus::Uncertain, "old nonzero sequence is not acknowledged after crash");
        const numeric::Event fresh{203, 0, numeric::EventKind::Sample, 11};
        const auto result = numeric::sendAcknowledged("restart:event", "restart:acknowledgedStats", fresh, 1, 5.0);
        testOk(result.status == numeric::SendStatus::Acknowledged && result.statistics[6] == 1 && result.statistics[0] == 11,
               "fresh session starts from empty statistics after crash");
    }
    return testDone();
}
