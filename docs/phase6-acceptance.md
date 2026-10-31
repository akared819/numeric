# Phase 6 acceptance and limits

The agreed Phase 6 endpoint is documented benchmarks, Linux and macOS validation, and a release-readiness checklist. Completion of this scope does not create a version, tag or release, guarantee performance, or claim comprehensive portability.

## Acceptance evidence

- [x] The native registry mean and isolated IOC record-processing benchmarks have reproducible commands, actual measurements, correctness checks and explicit timing boundaries in [benchmarks](benchmarks.md). Linux and macOS CI run functional smoke tests without speed thresholds.
- [x] Linux Ubuntu 24.04 and macOS arm64 CI build against clean EPICS Base 7.0.10 and external GSL and run native, IOC and example/network tests. Linux additionally runs GDB notification stress checks. PRs [#40](https://github.com/akared819/numeric/pull/40) and [#41](https://github.com/akared819/numeric/pull/41) passed on Ubuntu 24.04 with GSL 2.7.1 and macOS 14 arm64 with GSL 2.8.
- [x] The [release-readiness checklist](release-readiness.md) specifies candidate-specific compatibility, tests, safety, performance, licensing, provenance and approval checks. It is not a declaration that a particular release is ready.
- [x] The implementation PRs were reviewed and their required CI checks passed before merge.

The local benchmark examples were measured on Apple M3, macOS 26.6.2 with GSL 2.8 and the supplied EPICS Base development checkout. [PR #39](https://github.com/akared819/numeric/pull/39) records the native benchmark, [PR #40](https://github.com/akared819/numeric/pull/40) the platform workflow and C++14 portability fix, [PR #41](https://github.com/akared819/numeric/pull/41) the isolated IOC benchmark, and [PR #42](https://github.com/akared819/numeric/pull/42) the checklist. Each records its actual validation results.

## Boundaries and outstanding work

Measurements are single observations, not performance guarantees, statistically controlled comparisons or latency distributions. IOC measurements exclude network activity and do not instrument timed allocations. Other algorithms, production workloads, additional CPUs, operating systems and EPICS Base/GSL versions remain outside the tested scope. Each deployment must validate its own IOC and agree on the [process-wide GSL error-handler policy](architecture.md#gsl-setup-for-an-ioc).

The previously accepted intermittent EPICS Base notification crash remains unresolved; passing CI does not establish a fix. Its scope and evidence are documented in [Phase 2 acceptance](phase2-acceptance.md). GSL is not comprehensively wrapped; see [coverage](gsl-coverage.md). At Phase 6 completion no release version, artifact or tag had been chosen. Each release candidate must complete the checklist, including distribution and licensing review.
