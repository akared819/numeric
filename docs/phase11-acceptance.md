# Phase 11: implemented scope and validation

The 0.2.0 development scope provides representative capabilities in each
requested area, not complete GSL coverage or NumPy/SciPy compatibility.
The [coverage inventory](gsl-coverage.md) identifies direct GSL wrappers,
compositions and remaining gaps.

| Area | Representative capabilities |
| --- | --- |
| Statistics | Batch and running statistics, covariance/correlation, weighted mean, sorted quantile, moving statistics |
| FFT and signal processing | Complex mixed-radix and radix-2 transforms, real radix-2 forward/inverse, frequency axis, Hann window, raw power, Welch PSD, STFT |
| Numerical analysis | Linear and natural cubic interpolation, callback differentiation and adaptive integration, LU determinant, symmetric eigenvalues, nonlinear fitting, root finding, minimization and RKF45 ODE |
| Special functions and sampling | Gamma and erf, normal CDF, explicit-seed MT19937 uniform stream |

The native tests include analytical values, invalid inputs, non-finite values,
workspace reset and FFT round trips. Callback solver tests exercise failure and
iteration limits. Dedicated-record IOC tests cover numerical results, invalid
inputs, alarm and recovery for exposed operations; the example IOC demonstrates
Welch PSD and STFT over Channel Access. Native callback APIs and random streams
are not exposed as numeric EPICS fields. Conventions and limits are documented
in the linked operation guides.

On macOS 27.2 arm64 with EPICS Base R7.0.11 and GSL 2.8, the native suite,
initialized and uninitialized IOC suites, example IOC and network/reconnection
tests passed for the development code before the final documentation and
benchmark changes. The new example was also tested over Channel Access. Core
and FFT native tests passed with AddressSanitizer and UndefinedBehaviorSanitizer.
These observations do not replace final Linux, macOS and Windows CI on the exact
integration candidate. Final results belong in the PR and release record.
