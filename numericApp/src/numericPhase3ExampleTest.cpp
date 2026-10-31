// SPDX-License-Identifier: GPL-3.0-or-later
#include <cadef.h>
#include <epicsUnitTest.h>
#include <epicsEvent.h>
#include <dbEvent.h>
#include <cmath>
#include <cstring>

namespace {
epicsEventId arrayMonitorEvent = nullptr;
void arrayMonitor(event_handler_args args) {
    if (args.status != ECA_NORMAL || args.count != 5 || !args.dbr) return;
    const double* values = static_cast<const double*>(args.dbr);
    const double expected[] = {4.0/3,2,3,4,14.0/3};
    for (int i = 0; i < 5; ++i) if (std::fabs(values[i] - expected[i]) > 1e-10) return;
    epicsEventSignal(arrayMonitorEvent);
}
bool matches(const double* actual, const double* expected, unsigned long count) {
    for (unsigned long i = 0; i < count; ++i)
        if (!std::isfinite(actual[i]) || std::fabs(actual[i] - expected[i]) > 1e-10) return false;
    return true;
}
bool calculate(chid input, chid process, chid output, const double* samples,
               unsigned long inputCount, double* result, unsigned long resultCount) {
    dbr_long_t trigger = 1;
    return ca_array_put(DBR_DOUBLE, inputCount, input, samples) == ECA_NORMAL &&
           ca_put(DBR_LONG, process, &trigger) == ECA_NORMAL &&
           ca_array_get(DBR_DOUBLE, resultCount, output, result) == ECA_NORMAL &&
           ca_pend_io(10.0) == ECA_NORMAL;
}
}
int main() {
    testPlan(9);
    const char* names[] = {"demo:realFftInput", "demo:realFftForward.PROC", "demo:realFftOutput",
        "demo:fftInput", "demo:fftForward.PROC", "demo:fftOutput", "demo:fftInverse.PROC", "demo:ifftOutput",
        "demo:windowInput", "demo:movingMean.PROC", "demo:movingMeanOutput", "demo:movingMedian.PROC",
        "demo:movingMedianOutput", "demo:gaussianSmooth.PROC", "demo:gaussianOutput",
        "demo:arrayInput", "demo:arrayMean.PROC", "demo:arrayMean.VAL"};
    chid channels[sizeof(names)/sizeof(names[0])] = {};
    bool connected = ca_context_create(ca_enable_preemptive_callback) == ECA_NORMAL;
    for (std::size_t i = 0; connected && i < sizeof(names)/sizeof(names[0]); ++i)
        connected = ca_create_channel(names[i], nullptr, nullptr, CA_PRIORITY_DEFAULT, &channels[i]) == ECA_NORMAL;
    connected = connected && ca_pend_io(15.0) == ECA_NORMAL;
    testOk(connected, "Phase 3 example channels connect over Channel Access");
    double impulse[] = {1,0,0,0}, result[8] = {};
    const double spectrum[] = {1,0,1,0,1,0,1,0};
    testOk(connected && calculate(channels[0], channels[1], channels[2], impulse, 4, result, 8) &&
           matches(result, spectrum, 8), "real FFT impulse produces a full complex spectrum");
    double complexImpulse[] = {1,0,0,0,0,0,0,0};
    const bool forward = connected && calculate(channels[3], channels[4], channels[5], complexImpulse, 8, result, 8) && matches(result, spectrum, 8);
    dbr_long_t trigger = 1;
    const bool inverse = forward && ca_put(DBR_LONG, channels[6], &trigger) == ECA_NORMAL &&
        ca_array_get(DBR_DOUBLE, 8, channels[7], result) == ECA_NORMAL && ca_pend_io(10.0) == ECA_NORMAL;
    testOk(inverse && matches(result, complexImpulse, 8), "complex forward and inverse round trip");
    double samples[] = {1,2,3,4,5};
    const double means[] = {4.0/3,2,3,4,14.0/3};
    testOk(connected && calculate(channels[8], channels[9], channels[10], samples, 5, result, 5) &&
           matches(result, means, 5), "moving mean uses value-padded endpoints");
    double spikes[] = {1,1,99,1,1};
    const double medians[] = {1,1,1,1,1};
    testOk(connected && calculate(channels[8], channels[11], channels[12], spikes, 5, result, 5) &&
           matches(result, medians, 5), "moving median removes an isolated spike");
    double pulse[] = {0,0,1,0,0};
    const double smooth[] = {0,0.25,0.5,0.25,0};
    testOk(connected && calculate(channels[8], channels[13], channels[14], pulse, 5, result, 5) &&
           matches(result, smooth, 5), "Gaussian smoothing produces the expected impulse response");
    evid subscription = nullptr;
    arrayMonitorEvent = epicsEventMustCreate(epicsEventEmpty);
    const bool subscribed = connected &&
        ca_create_subscription(DBR_DOUBLE, 0, channels[17], DBE_VALUE | DBE_ALARM,
                               arrayMonitor, nullptr, &subscription) == ECA_NORMAL &&
        ca_flush_io() == ECA_NORMAL;
    const bool arrayComputed = subscribed && calculate(channels[15], channels[16], channels[17], samples, 5, result, 5) &&
        matches(result, means, 5);
    testOk(arrayComputed, "dedicated array record publishes moving mean over Channel Access");
    testOk(arrayComputed && epicsEventWaitWithTimeout(arrayMonitorEvent, 10.0) == epicsEventWaitOK,
           "dedicated array record posts a Channel Access monitor");
    double overwrite[] = {9,9,9,9,9};
    const int putStatus = arrayComputed ? ca_array_put(DBR_DOUBLE, 5, channels[17], overwrite) : ECA_INTERNAL;
    const bool rejected = arrayComputed && (putStatus == ECA_NORMAL || putStatus == ECA_NOWTACCESS) &&
        ca_array_get(DBR_DOUBLE, 5, channels[17], result) == ECA_NORMAL && ca_pend_io(10.0) == ECA_NORMAL &&
        matches(result, means, 5);
    if (!rejected) testDiag("put status %s, observed first element %.17g", ca_message(putStatus), result[0]);
    testOk(rejected, "array result rejects a Channel Access write");
    if (subscription) ca_clear_subscription(subscription);
    for (chid channel : channels) if (channel) ca_clear_channel(channel);
    if (ca_current_context()) ca_context_destroy();
    epicsEventDestroy(arrayMonitorEvent);
    return testDone();
}
