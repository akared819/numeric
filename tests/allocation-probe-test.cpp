// SPDX-License-Identifier: GPL-3.0-or-later
#include <dlfcn.h>
#include <gsl/gsl_errno.h>
#include <gsl/gsl_rng.h>
#include <cstdio>
int main() {
    using Begin = void (*)();
    using End = unsigned long long (*)();
    auto begin = reinterpret_cast<Begin>(dlsym(RTLD_DEFAULT, "numericAllocationBegin"));
    auto end = reinterpret_cast<End>(dlsym(RTLD_DEFAULT, "numericAllocationEnd"));
    if (!begin || !end) return 1;
    gsl_set_error_handler_off();
    begin(); gsl_rng* rng = gsl_rng_alloc(gsl_rng_mt19937); const auto count = end();
    if (rng) gsl_rng_free(rng);
    std::printf("gsl_rng_allocation_calls=%llu\n", count);
    return rng && count ? 0 : 1;
}
