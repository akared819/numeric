# numeric 0.2.0 development notes

This document describes the development candidate; it is not a declaration
that version 0.2.0 has been released. Final commit, platform results and release
approval must be recorded against the actual candidate.

## Changes

- Operation metadata describes argument types, defaults and validation.
  Dedicated records provide runtime `ARGA`–`ARGP` and `INPA`–`INPP` fields;
  legacy fields and links remain available under documented precedence rules.
- Native typed array views describe shape, strides and six real, integer and
  complex element formats. Dedicated records accept supported real waveform
  types; complex and multidimensional views remain native interfaces.
- New functionality includes weighted means, sorted quantiles, real inverse
  and mixed-radix FFT, frequency axes, Hann windows, raw power, Welch PSD,
  STFT, natural cubic splines, matrix determinant and symmetric eigenvalues.
- Native callback interfaces provide adaptive differentiation and integration,
  Brent root finding and minimization, RKF45 ODE solving and nonlinear least
  squares. A separately owned seeded stream supports GSL uniform sampling.
  Callback interfaces are not EPICS fields or record operations.

See [coverage](gsl-coverage.md), [scalar](numeric-scalar.md),
[array](numeric-array.md), [FFT](fft.md), [callback solvers](numerical-differentiation.md),
[fitting](nonlinear-fitting.md) and [sampling](random-sampling.md) for the
supported input, output, failure and resource rules.

## Upgrade and validation

Rebuild numeric and regenerate/rebuild the application's DBD and record
support together when upgrading: the dedicated record layouts changed. Existing
databases retain the legacy interface, subject to the documented `INP`/`INP*`
and legacy field/`ARG*` conflict rules. Inputs linked from separate PVs are read
sequentially and are not an atomic snapshot. INT64 through Channel Access may
already have lost precision before numeric receives it; local INT64 links are
checked exactly. Input updates do not automatically process records.

GSL 2.8 or newer and EPICS Base 7 are required; build and installation steps
are in the [README](../README.md). Python is not required at runtime. Each IOC
must configure the documented non-terminating [GSL error-handler policy](architecture.md#gsl-setup-for-an-ioc)
before numerical work. An INVALID alarm may leave a previous result visible.
The known intermittent EPICS Base notification crash remains an external
limitation; it has not been shown fixed by successful tests. In the aSub
interface, automatic input fetch failures may occur before the numeric callback
and do not always produce the expected alarm; monitor input freshness separately.
Test the intended IOC configuration before production deployment.

Final validation must cover the exact candidate commit on Linux, macOS and
Windows, the required native and IOC suites, compatibility, licensing and the
[release-readiness checklist](release-readiness.md). Measurements and their
limits are in [benchmarks](benchmarks.md); they do not establish hard real-time
behavior. The project is GPL-3.0-or-later and GSL is an external GPL dependency;
see [licensing](licensing.md).

The [Phase 11 scope and test summary](phase11-acceptance.md) records the
representative functionality and the local validation performed so far.
