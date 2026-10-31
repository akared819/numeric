// SPDX-License-Identifier: GPL-3.0-or-later
#include "numericAckClient.h"
#include <cadef.h>
#include <epicsUnitTest.h>
#include <cmath>

int main() {
    testPlan(11);
    chid input = nullptr, process = nullptr, result = nullptr;
    chid pairs = nullptr, pairsProcess = nullptr, pairsResult = nullptr;
    const bool connected = ca_context_create(ca_enable_preemptive_callback) == ECA_NORMAL &&
        ca_create_channel("demo:input", nullptr, nullptr, CA_PRIORITY_DEFAULT, &input) == ECA_NORMAL &&
        ca_create_channel("demo:mean.PROC", nullptr, nullptr, CA_PRIORITY_DEFAULT, &process) == ECA_NORMAL &&
        ca_create_channel("demo:result", nullptr, nullptr, CA_PRIORITY_DEFAULT, &result) == ECA_NORMAL &&
        ca_create_channel("demo:covariancePairsInput", nullptr, nullptr, CA_PRIORITY_DEFAULT, &pairs) == ECA_NORMAL &&
        ca_create_channel("demo:covariancePairs.PROC", nullptr, nullptr, CA_PRIORITY_DEFAULT, &pairsProcess) == ECA_NORMAL &&
        ca_create_channel("demo:covariancePairsResult", nullptr, nullptr, CA_PRIORITY_DEFAULT, &pairsResult) == ECA_NORMAL &&
        ca_pend_io(15.0) == ECA_NORMAL;
    testOk(connected, "example IOC batch channels connect over Channel Access");
    double samples[] = {1, 3, 5};
    dbr_long_t trigger = 1;
    double value = 0;
    const bool computed = connected && ca_array_put(DBR_DOUBLE, 3, input, samples) == ECA_NORMAL &&
        ca_put(DBR_LONG, process, &trigger) == ECA_NORMAL && ca_array_get(DBR_DOUBLE, 1, result, &value) == ECA_NORMAL &&
        ca_pend_io(10.0) == ECA_NORMAL && value == 3;
    testOk(computed, "example IOC computes mean from an explicitly processed waveform");
    double full[] = {1, 2, 3, 4, 5, 6, 7, 8};
    value = 0;
    const bool fullComputed = connected && ca_array_put(DBR_DOUBLE, 8, input, full) == ECA_NORMAL &&
        ca_put(DBR_LONG, process, &trigger) == ECA_NORMAL && ca_array_get(DBR_DOUBLE, 1, result, &value) == ECA_NORMAL &&
        ca_pend_io(10.0) == ECA_NORMAL && value == 4.5;
    testOk(fullComputed, "example mean accepts the full source waveform without truncation");
    double packed[] = {1, 2, 3, 4, 5, 6};
    value = 0;
    const bool pairedComputed = connected && ca_array_put(DBR_DOUBLE, 6, pairs, packed) == ECA_NORMAL &&
        ca_put(DBR_LONG, pairsProcess, &trigger) == ECA_NORMAL && ca_array_get(DBR_DOUBLE, 1, pairsResult, &value) == ECA_NORMAL &&
        ca_pend_io(10.0) == ECA_NORMAL && value == 4;
    testOk(pairedComputed, "example IOC calculates covariance from paired waveform data");
    chid scalarInput = nullptr, scalarProcess = nullptr, scalarResult = nullptr;
    const bool scalarConnected = connected &&
        ca_create_channel("demo:scalarInput", nullptr, nullptr, CA_PRIORITY_DEFAULT, &scalarInput) == ECA_NORMAL &&
        ca_create_channel("demo:scalarMean.PROC", nullptr, nullptr, CA_PRIORITY_DEFAULT, &scalarProcess) == ECA_NORMAL &&
        ca_create_channel("demo:scalarMean.VAL", nullptr, nullptr, CA_PRIORITY_DEFAULT, &scalarResult) == ECA_NORMAL &&
        ca_pend_io(10.0) == ECA_NORMAL;
    double scalarSamples[] = {1, 2, 3, 4}, scalarValue = 0;
    const bool scalarComputed = scalarConnected && ca_array_put(DBR_DOUBLE, 4, scalarInput, scalarSamples) == ECA_NORMAL &&
        ca_put(DBR_LONG, scalarProcess, &trigger) == ECA_NORMAL &&
        ca_array_get(DBR_DOUBLE, 1, scalarResult, &scalarValue) == ECA_NORMAL &&
        ca_pend_io(10.0) == ECA_NORMAL && scalarValue == 2.5;
    testOk(scalarComputed, "example dedicated scalar record computes mean over Channel Access");
    chid normProcess = nullptr, normResult = nullptr;
    const bool normConnected = scalarConnected &&
        ca_create_channel("demo:vectorNorm.PROC", nullptr, nullptr, CA_PRIORITY_DEFAULT, &normProcess) == ECA_NORMAL &&
        ca_create_channel("demo:vectorNorm.VAL", nullptr, nullptr, CA_PRIORITY_DEFAULT, &normResult) == ECA_NORMAL &&
        ca_pend_io(10.0) == ECA_NORMAL;
    double normValue = 0;
    const bool normComputed = normConnected && ca_put(DBR_LONG, normProcess, &trigger) == ECA_NORMAL &&
        ca_array_get(DBR_DOUBLE, 1, normResult, &normValue) == ECA_NORMAL && ca_pend_io(10.0) == ECA_NORMAL &&
        std::fabs(normValue - std::sqrt(30.0)) < 1e-12;
    testOk(normComputed, "example record computes vector norm over Channel Access");
    if (normProcess) ca_clear_channel(normProcess);
    if (normResult) ca_clear_channel(normResult);
    chid erfProcess = nullptr, erfResult = nullptr;
    const bool erfConnected = connected &&
        ca_create_channel("demo:erf.PROC", nullptr, nullptr, CA_PRIORITY_DEFAULT, &erfProcess) == ECA_NORMAL &&
        ca_create_channel("demo:erf.VAL", nullptr, nullptr, CA_PRIORITY_DEFAULT, &erfResult) == ECA_NORMAL &&
        ca_pend_io(10.0) == ECA_NORMAL;
    double erfValue = 0;
    const bool erfComputed = erfConnected && ca_put(DBR_LONG, erfProcess, &trigger) == ECA_NORMAL &&
        ca_array_get(DBR_DOUBLE, 1, erfResult, &erfValue) == ECA_NORMAL && ca_pend_io(10.0) == ECA_NORMAL &&
        std::fabs(erfValue - 0.8427007929497149) < 1e-12;
    testOk(erfComputed, "example record computes erf over Channel Access");
    chid cdfProcess = nullptr, cdfResult = nullptr;
    const bool cdfConnected = connected &&
        ca_create_channel("demo:normalCdf.PROC", nullptr, nullptr, CA_PRIORITY_DEFAULT, &cdfProcess) == ECA_NORMAL &&
        ca_create_channel("demo:normalCdf.VAL", nullptr, nullptr, CA_PRIORITY_DEFAULT, &cdfResult) == ECA_NORMAL &&
        ca_pend_io(10.0) == ECA_NORMAL;
    double cdfValue = 0;
    const bool cdfComputed = cdfConnected && ca_put(DBR_LONG, cdfProcess, &trigger) == ECA_NORMAL &&
        ca_array_get(DBR_DOUBLE, 1, cdfResult, &cdfValue) == ECA_NORMAL && ca_pend_io(10.0) == ECA_NORMAL &&
        std::fabs(cdfValue - 0.8413447460685429) < 1e-12;
    testOk(cdfComputed, "example record computes standard normal CDF over Channel Access");
    if (cdfProcess) ca_clear_channel(cdfProcess);
    if (cdfResult) ca_clear_channel(cdfResult);
    if (erfProcess) ca_clear_channel(erfProcess);
    if (erfResult) ca_clear_channel(erfResult);
    if (scalarInput) ca_clear_channel(scalarInput);
    if (scalarProcess) ca_clear_channel(scalarProcess);
    if (scalarResult) ca_clear_channel(scalarResult);
    if (input) ca_clear_channel(input);
    if (process) ca_clear_channel(process);
    if (result) ca_clear_channel(result);
    if (pairs) ca_clear_channel(pairs);
    if (pairsProcess) ca_clear_channel(pairsProcess);
    if (pairsResult) ca_clear_channel(pairsResult);
    if (ca_current_context()) ca_context_destroy();
    const numeric::Event first{71, 0, numeric::EventKind::Sample, 2};
    const auto a = numeric::sendAcknowledged("demo:event", "demo:acknowledgedStats", first, 1, 10.0);
    testOk(a.status == numeric::SendStatus::Acknowledged && a.statistics[6] == 1 && a.statistics[0] == 2,
           "example IOC acknowledges its first streaming sample");
    const numeric::Event second{71, 1, numeric::EventKind::Sample, 4};
    const auto b = numeric::sendAcknowledged("demo:event", "demo:acknowledgedStats", second, 1, 10.0);
    testOk(b.status == numeric::SendStatus::Acknowledged && b.statistics[6] == 2 && b.statistics[0] == 3,
           "example IOC updates streaming statistics");
    const numeric::Event reset{71, 2, numeric::EventKind::Reset, 0};
    const auto c = numeric::sendAcknowledged("demo:event", "demo:acknowledgedStats", reset, 1, 10.0);
    testOk(c.status == numeric::SendStatus::Acknowledged && c.statistics[6] == 0,
           "example IOC acknowledges an ordered reset");
    return testDone();
}
