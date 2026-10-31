// SPDX-License-Identifier: GPL-3.0-or-later
#include "numericAckClient.h"
#include <epicsUnitTest.h>
#include <cstdio>
#include <cstring>

int main(int argc, char** argv) {
    if (argc != 2 || (std::strcmp(argv[1], "prepare") && std::strcmp(argv[1], "inflight") && std::strcmp(argv[1], "recovery") && std::strcmp(argv[1], "post-inflight") && std::strcmp(argv[1], "post-recovery"))) {
        std::fprintf(stderr, "Usage: numericCrashTest prepare|inflight|recovery|post-inflight|post-recovery\n");
        return 2;
    }
    if (!std::strcmp(argv[1], "prepare")) {
        testPlan(2);
        const numeric::Event first{81, 0, numeric::EventKind::Sample, 1};
        const auto one = numeric::sendAcknowledged("numericNet:event", "numericNet:stats", first, 1, 10.0);
        testOk(one.status == numeric::SendStatus::Acknowledged && one.statistics[6] == 1,
               "crash stream accepts its first sample after the earlier restart");
        const numeric::Event second{81, 1, numeric::EventKind::Sample, 3};
        const auto two = numeric::sendAcknowledged("numericNet:event", "numericNet:stats", second, 1, 10.0);
        testOk(two.status == numeric::SendStatus::Acknowledged && two.statistics[6] == 2,
               "crash stream accepts its second sample before the fault");
    } else if (!std::strcmp(argv[1], "inflight")) {
        testPlan(1);
        // The prepare phase accepted sequence 1 in session 81 on this IOC.
        const numeric::Event event{81, 2, numeric::EventKind::Sample, 42};
        const auto result = numeric::sendAcknowledged("numericNet:event", "numericNet:stats", event, 1, 10.0);
        testOk(result.status == numeric::SendStatus::Uncertain,
               "sender reports uncertainty when IOC dies during event processing");
    } else if (!std::strcmp(argv[1], "recovery")) {
        testPlan(2);
        const numeric::Event stale{202, 1, numeric::EventKind::Sample, 10};
        const auto old = numeric::sendAcknowledged("restart:event", "restart:acknowledgedStats", stale, 1, 1.0);
        testOk(old.status == numeric::SendStatus::Uncertain, "old nonzero sequence is not acknowledged after crash");
        const numeric::Event fresh{203, 0, numeric::EventKind::Sample, 11};
        const auto result = numeric::sendAcknowledged("restart:event", "restart:acknowledgedStats", fresh, 1, 5.0);
        testOk(result.status == numeric::SendStatus::Acknowledged && result.statistics[6] == 1 && result.statistics[0] == 11,
               "fresh session starts from empty statistics after crash");
    } else if (!std::strcmp(argv[1], "post-inflight")) {
        testPlan(1);
        const numeric::Event event{901, 0, numeric::EventKind::Sample, 7};
        const auto result = numeric::sendAcknowledged("numericPost:event", "numericPost:stats", event, 1, 10.0);
        testOk(result.status == numeric::SendStatus::Uncertain,
               "sender reports uncertainty when IOC dies after numerical update");
    } else {
        testPlan(2);
        const numeric::Event event{901, 0, numeric::EventKind::Sample, 7};
        const auto replay = numeric::sendAcknowledged("numericPost:event", "numericPost:stats", event, 1, 5.0);
        testOk(replay.status == numeric::SendStatus::Acknowledged && replay.statistics[6] == 1 && replay.statistics[0] == 7,
               "old first event is accepted again after state is lost");
        const numeric::Event fresh{204, 0, numeric::EventKind::Sample, 13};
        const auto result = numeric::sendAcknowledged("restart:event", "restart:acknowledgedStats", fresh, 1, 5.0);
        testOk(result.status == numeric::SendStatus::Acknowledged && result.statistics[6] == 1 && result.statistics[0] == 13,
               "a distinct fresh session starts empty after the crash");
    }
    return testDone();
}
