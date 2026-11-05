// SPDX-License-Identifier: GPL-3.0-or-later
// Native examples for the bilingual API reference. Check every Result before
// consuming outputs; none of these callbacks are exposed as EPICS PV fields.
#include "numeric.h"
#include <cmath>
#include <cstdio>

using namespace numeric;

double square(double x, void*) { return x * x; }
double rootFn(double x, void*) { return x * x - 2; }
double minimumFn(double x, void*) { return (x - 2) * (x - 2); }

bool ode(double, const double* x, double* dx, std::size_t n, void*) {
    if (n != 1) return false;
    dx[0] = x[0];
    return true;
}

bool residual(const double* p, std::size_t np, double* r, std::size_t nr, void*) {
    if (np != 2 || nr != 3) return false;
    const double y[] = {2, 4, 8};
    for (std::size_t i = 0; i < nr; ++i)
        r[i] = p[0] * std::exp(p[1] * static_cast<double>(i)) - y[i];
    return true;
}

int main() {
    initializeGsl();  // Before concurrent use and other GSL calls in the process.
    double value = 0, error = 0;
    Result result = differentiateCentral(square, nullptr, 2, 0.01, value, error);
    if (result.status != Status::Ok) return 1;
    std::printf("differentiate: %.9g\n", value);  // 4

    result = integrateAdaptive(square, nullptr, 0, 2, 1e-9, 1e-9, 100, value, error);
    if (result.status != Status::Ok) return 1;
    std::printf("integrate: %.9g\n", value);  // 8/3

    result = findRootBrent(rootFn, nullptr, 0, 2, 1e-12, 1e-12, 100, value, error);
    if (result.status != Status::Ok) return 1;
    std::printf("root: %.9g\n", value);  // sqrt(2)

    result = minimizeBrent(minimumFn, nullptr, 0, 1, 4, 1e-6, 1e-6, 100, value, error);
    if (result.status != Status::Ok) return 1;
    std::printf("minimum: %.9g\n", value);  // 2

    double initial[] = {1}, output[1] = {};
    result = solveOdeRkf45(ode, nullptr, 0, 1, initial, 1, 0.01, 1e-9, 1e-9, 1000, output, 1);
    if (result.status != Status::Ok) return 1;
    std::printf("ODE: %.9g\n", output[0]);  // exp(1)

    double guess[] = {1.8, 0.5}, parameters[2] = {}, sumsq = 0;
    result = fitNonlinear(residual, nullptr, guess, 2, 3, 100, 1e-8, parameters, 2, sumsq);
    if (result.status != Status::Ok) return 1;
    std::printf("fit: %.9g %.9g\n", parameters[0], parameters[1]);  // 2, log(2)
    return 0;
}
