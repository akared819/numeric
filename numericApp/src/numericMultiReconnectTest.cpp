// SPDX-License-Identifier: GPL-3.0-or-later
#include <alarm.h>
#include <cadef.h>
#include <epicsThread.h>
#include <epicsUnitTest.h>
#include <cmath>
#include <cstring>

int main(int argc, char** argv) {
    if (argc != 2 || (std::strcmp(argv[1], "initial") && std::strcmp(argv[1], "down") && std::strcmp(argv[1], "recover"))) return 2;
    const bool down = std::strcmp(argv[1], "down") == 0;
    const bool recover = std::strcmp(argv[1], "recover") == 0;
    testPlan(3);
    const char* names[] = {"multiReconnect:local", "multiReconnect:correlation.PROC", "multiReconnect:correlation.VAL",
                           "multiReconnect:correlation.SEVR", "multiReconnect:fit.PROC", "multiReconnect:fit.VAL",
                           "multiReconnect:fit.SEVR", "multiReconnect:floatMean.PROC", "multiReconnect:floatMean.VAL",
                           "multiReconnect:floatMean.SEVR", "multiReconnect:remote", "multiReconnect:remoteFloat"};
    chid channels[12] = {};
    bool connected = ca_context_create(ca_enable_preemptive_callback) == ECA_NORMAL;
    const std::size_t channelCount = down ? 10 : 12;
    for (std::size_t i = 0; connected && i < channelCount; ++i)
        connected = ca_create_channel(names[i], nullptr, nullptr, CA_PRIORITY_DEFAULT, &channels[i]) == ECA_NORMAL;
    connected = connected && ca_pend_io(15.0) == ECA_NORMAL;
    testOk(connected, "consumer and required producer channels connect");
    double x[] = {1, 2, 3, 4}, positive[] = {2, 4, 6, 8}, negative[] = {-2, -4, -6, -8};
    float positiveFloat[] = {1, 2, 3, 4}, negativeFloat[] = {-1, -2, -3, -4};
    const bool sent = connected && (down ||
        (ca_array_put(DBR_DOUBLE, 4, channels[0], x) == ECA_NORMAL &&
         ca_array_put(DBR_DOUBLE, 4, channels[10], recover ? negative : positive) == ECA_NORMAL &&
         ca_array_put(DBR_FLOAT, 4, channels[11], recover ? negativeFloat : positiveFloat) == ECA_NORMAL &&
         ca_flush_io() == ECA_NORMAL));
    testOk(sent, "inputs are sent when producer is available");
    bool correct = false;
    for (int attempt = 0; sent && attempt < 120 && !correct; ++attempt) {
        dbr_long_t trigger = 1; dbr_short_t scalarSeverity = -1, arraySeverity = -1, floatSeverity = -1;
        double correlation = 0, fit[6] = {}, floatMean = 0;
        const bool read = ca_put(DBR_LONG, channels[1], &trigger) == ECA_NORMAL &&
            ca_put(DBR_LONG, channels[4], &trigger) == ECA_NORMAL &&
            ca_put(DBR_LONG, channels[7], &trigger) == ECA_NORMAL &&
            ca_get(DBR_SHORT, channels[3], &scalarSeverity) == ECA_NORMAL &&
            ca_get(DBR_SHORT, channels[6], &arraySeverity) == ECA_NORMAL &&
            ca_get(DBR_SHORT, channels[9], &floatSeverity) == ECA_NORMAL &&
            ca_get(DBR_DOUBLE, channels[2], &correlation) == ECA_NORMAL &&
            ca_get(DBR_DOUBLE, channels[8], &floatMean) == ECA_NORMAL &&
            ca_array_get(DBR_DOUBLE, 6, channels[5], fit) == ECA_NORMAL && ca_pend_io(2.0) == ECA_NORMAL;
        const double expectedCorrelation = recover ? -1 : 1;
        const double expectedSlope = recover ? -2 : 2;
        const double expectedFit[] = {0, expectedSlope, 0, 0, 0, 0};
        bool fitMatches = true;
        for (std::size_t i = 0; i < 6; ++i)
            if (!std::isfinite(fit[i]) || std::fabs(fit[i] - expectedFit[i]) >= 1e-12) fitMatches = false;
        correct = read && scalarSeverity == (down ? INVALID_ALARM : NO_ALARM) &&
            arraySeverity == (down ? INVALID_ALARM : NO_ALARM) &&
            floatSeverity == (down ? INVALID_ALARM : NO_ALARM) &&
            std::fabs(floatMean - (recover ? -2.5 : 2.5)) < 1e-12 &&
            std::fabs(correlation - expectedCorrelation) < 1e-12 && fitMatches;
        if (!correct) epicsThreadSleep(0.25);
    }
    testOk(correct, down ? "disconnected links alarm and retain prior results" :
                          (recover ? "unchanged links reconnect and use new producer values" : "all records compute initial values"));
    for (chid channel : channels) if (channel) ca_clear_channel(channel);
    if (ca_current_context()) ca_context_destroy();
    return testDone();
}
