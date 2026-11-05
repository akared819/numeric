// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef NUMERIC_LATENCY_H
#define NUMERIC_LATENCY_H
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <vector>

// Times individual calls after warmup. Measurement includes clock overhead.
template<typename Function>
bool numericSampleLatency(const char* name, std::size_t count, Function function) {
    std::vector<double> samples(count);
    using Clock = std::chrono::steady_clock;
    for (std::size_t i = 0; i < count; ++i) {
        const auto start = Clock::now();
        if (!function()) return false;
        const auto end = Clock::now();
        samples[i] = std::chrono::duration<double, std::nano>(end - start).count();
    }
    std::sort(samples.begin(), samples.end());
    std::printf("%s_latency_samples=%zu\n%s_p50_ns=%.1f\n%s_p95_ns=%.1f\n%s_p99_ns=%.1f\n%s_max_ns=%.1f\n",
                name, count, name, samples[(count-1)*50/100], name, samples[(count-1)*95/100],
                name, samples[(count-1)*99/100], name, samples.back());
    return true;
}
#endif
