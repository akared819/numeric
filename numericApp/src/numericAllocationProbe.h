// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef NUMERIC_ALLOCATION_PROBE_H
#define NUMERIC_ALLOCATION_PROBE_H
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#ifdef __APPLE__
#include <dlfcn.h>
#endif

template<typename Function>
bool numericCountAllocations(const char* name, std::size_t count, Function function) {
#ifdef __APPLE__
    using Begin = void (*)();
    using End = unsigned long long (*)();
    const auto begin = reinterpret_cast<Begin>(dlsym(RTLD_DEFAULT, "numericAllocationBegin"));
    const auto end = reinterpret_cast<End>(dlsym(RTLD_DEFAULT, "numericAllocationEnd"));
    if (begin && end) {
        void* (*volatile allocate)(std::size_t) = &std::malloc;
        begin(); void* probe = allocate(16); const auto probeCalls = end();
        std::free(probe);
        if (!probe || probeCalls == 0) { std::fprintf(stderr, "allocation probe self-test failed\n"); return false; }
        begin();
        for (std::size_t i = 0; i < count; ++i) if (!function()) { end(); return false; }
        const auto calls = end();
        std::printf("%s_allocation_calls=%llu\n%s_allocation_iterations=%zu\n", name, calls, name, count);
        return true;
    }
#else
    (void)name; (void)count; (void)function;
#endif
    return true;
}
#endif
