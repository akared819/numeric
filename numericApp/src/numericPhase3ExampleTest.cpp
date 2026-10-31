// SPDX-License-Identifier: GPL-3.0-or-later
#include <cadef.h>
#include <epicsUnitTest.h>
#include <cmath>
#include <cstring>

namespace {
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
    testPlan(6);
    const char* names[] = {"demo:realFftInput", "demo:realFftForward.PROC", "demo:realFftOutput",
        "demo:fftInput", "demo:fftForward.PROC", "demo:fftOutput", "demo:fftInverse.PROC", "demo:ifftOutput",
        "demo:windowInput", "demo:movingMean.PROC", "demo:movingMeanOutput", "demo:movingMedian.PROC",
        "demo:movingMedianOutput", "demo:gaussianSmooth.PROC", "demo:gaussianOutput"};
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
    for (chid channel : channels) if (channel) ca_clear_channel(channel);
    if (ca_current_context()) ca_context_destroy();
    return testDone();
}
