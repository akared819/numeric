// SPDX-License-Identifier: GPL-3.0-or-later
#include "numericAckClient.h"
#include <cadef.h>
#include <epicsUnitTest.h>
#include <cstdio>
#include <cstring>

int main(int argc, char** argv) {
    if (argc != 2 || (std::strcmp(argv[1], "first") && std::strcmp(argv[1], "observe") && std::strcmp(argv[1], "blocked") && std::strcmp(argv[1], "retry"))) {
        std::fprintf(stderr, "Usage: numericPartitionTest first|observe|blocked|retry\n");
        return 2;
    }
    const numeric::Event event{701, 0, numeric::EventKind::Sample, 7};
    if (!std::strcmp(argv[1], "first")) {
        testPlan(1);
        const auto result = numeric::sendAcknowledged("numericPartition:event", "numericPartition:stats", event, 1, 5.0);
        testOk(result.status == numeric::SendStatus::Uncertain, "sender reports uncertainty after relay connection is cut");
    } else if (!std::strcmp(argv[1], "observe")) {
        testPlan(1);
        chid channel = nullptr;
        double values[11]{};
        bool ok = ca_context_create(ca_enable_preemptive_callback) == ECA_NORMAL &&
            ca_create_channel("numericPartition:stats.VALL", nullptr, nullptr, CA_PRIORITY_DEFAULT, &channel) == ECA_NORMAL &&
            ca_pend_io(10.0) == ECA_NORMAL && ca_array_get(DBR_DOUBLE, 11, channel, values) == ECA_NORMAL &&
            ca_pend_io(10.0) == ECA_NORMAL && values[0] == 7 && values[6] == 1 && values[7] == 701 &&
            values[8] == 0 && values[9] == 0 && values[10] == 7;
        testOk(ok, "independent direct CA client verifies the IOC accepted the original event");
        if (channel) ca_clear_channel(channel);
        if (ca_current_context()) ca_context_destroy();
    } else if (!std::strcmp(argv[1], "blocked")) {
        testPlan(1);
        const numeric::Event next{701, 1, numeric::EventKind::Sample, 9};
        const auto result = numeric::sendAcknowledged("numericPartition:event", "numericPartition:stats", next, 1, 1.0);
        testOk(result.status == numeric::SendStatus::Uncertain, "a new event cannot be acknowledged while the relay is down");
    } else {
        testPlan(2);
        const auto retry = numeric::sendAcknowledged("numericPartition:event", "numericPartition:stats", event, 1, 10.0);
        testOk(retry.status == numeric::SendStatus::Acknowledged && retry.statistics[6] == 1 && retry.statistics[0] == 7,
               "reconnecting and retrying the event does not count it twice");
        const numeric::Event next{701, 1, numeric::EventKind::Sample, 9};
        const auto result = numeric::sendAcknowledged("numericPartition:event", "numericPartition:stats", next, 1, 10.0);
        testOk(result.status == numeric::SendStatus::Acknowledged && result.statistics[6] == 2 && result.statistics[0] == 8,
               "next event is accepted after the interrupted connection");
    }
    return testDone();
}
