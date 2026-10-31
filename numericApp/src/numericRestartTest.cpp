// SPDX-License-Identifier: GPL-3.0-or-later
#include "numericAckClient.h"
#include <epicsUnitTest.h>
#include <cstring>
#include <cstdio>

static numeric::SendResult send(const char* prefix, std::uint64_t session, std::uint64_t sequence, double value) {
    const numeric::Event event{session, sequence, numeric::EventKind::Sample, value};
    return numeric::sendAcknowledged(std::string(prefix) + "event",
        std::string(prefix) + "acknowledgedStats", event, 1, 1.0);
}

int main(int argc, char** argv) {
    if (argc != 2 || (std::strcmp(argv[1], "before") && std::strcmp(argv[1], "after"))) {
        std::fprintf(stderr, "Usage: numericRestartTest before|after\n");
        return 2;
    }
    testPlan(3);
    if (!std::strcmp(argv[1], "before")) {
        const auto first = send("restart:", 201, 0, 4);
        testOk(first.status == numeric::SendStatus::Acknowledged && first.statistics[6] == 1,
               "first sample is acknowledged before restart");
        const auto second = send("restart:", 201, 1, 8);
        testOk(second.status == numeric::SendStatus::Acknowledged && second.statistics[6] == 2 && second.statistics[0] == 6,
               "second sample updates the original stream");
        const auto ambiguous = send("ambiguous:", 301, 0, 5);
        testOk(ambiguous.status == numeric::SendStatus::Acknowledged && ambiguous.statistics[6] == 1,
               "separate stream accepts its first sample before restart");
    } else {
        const auto stale = send("restart:", 201, 2, 12);
        testOk(stale.status == numeric::SendStatus::Uncertain,
               "old nonzero sequence is not acknowledged after restart");
        const auto fresh = send("restart:", 202, 0, 9);
        testOk(fresh.status == numeric::SendStatus::Acknowledged && fresh.statistics[6] == 1 && fresh.statistics[0] == 9,
               "new session starts with empty statistics after restart");
        const auto reused = send("ambiguous:", 301, 0, 5);
        testOk(reused.status == numeric::SendStatus::Acknowledged && reused.statistics[6] == 1,
               "reused first event is indistinguishable from a new stream after restart");
    }
    return testDone();
}
