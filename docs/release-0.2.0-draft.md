# numeric 0.2.0 release notes

These notes describe version 0.2.0. See [GitHub releases](https://github.com/akared819/numeric/releases)
for its publication status, exact source commit, validation and artifacts.

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
- The spectral magnitude calculation uses GSL `gsl_hypot` to address a
  `std::hypot` recursion observed with one Windows MinGW toolchain. The Windows
  test runner applies per-test timeouts.
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
The [0.2.0 Linux validation](linux-validation-0.2.0.md) records the separate
development-branch run on Ubuntu 26.04.1.

## User PC validation

The maintainer reported testing on Windows 11 with EPICS Base 7.0.10 and on
Ubuntu 26.04.1 with GCC 15.2, EPICS Base 7.0.10 and GSL 2.8. The recorded
[Windows validation](windows.md#develop-validation-2026-11-05) additionally
identifies GSL 2.8 and MinGW-w64 GCC 9.1.0, documents the spectral calculation
issue found on that PC and the passing test run after its fix. The
[Linux validation](linux-validation-0.2.0.md) records the tested development
commit, detailed test scope and results. These records describe the tested
hosts and commits; they do not automatically verify a later release commit or
other deployments.

## Publication preparation record

- [x] Record reported Windows and Linux host versions and link the detailed
  test records, including the Windows spectral fix and its regression tests.
- [x] Prepare the upgrade notes, known limitations, licensing references and
  user guide for record inputs and arguments.
- Consult the GitHub release for the final commit, CI results, publication date,
  source distribution and checksum. Binary distributions are not included.
