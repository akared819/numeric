# Roadmap and acceptance

## This increment: Phase 0 and minimal Phase 1

- [x] EPICS module layout, configurable Base and GSL, native test application, Linux CI definition.
- [x] Registry, metadata, descriptors, operation lifecycle, validation and GSL mean adapter.
- [x] Architecture, coverage inventory and licensing considerations.
- [x] Linux CI built and tested Phase 1 commit `3c29004` using EPICS Base 7.0.10.
- [x] Choose GPL-3.0-or-later for numeric and document GSL linking and redistribution considerations.
- [x] The initial commit built and tested against the clean EPICS Base 7.0.10 release on Linux CI.

Local verification: `make GSL_DIR=/tmp/numeric-gsl-install` and `./bin/darwin-aarch64/numericTest` passed on macOS using GSL 2.8 and EPICS Base R7.0.10-151-g0cc912b17-dirty. Phase 1 commit `3c29004` also passed Linux CI with the clean Base 7.0.10 release. The reproducible test command is in the README.

## Next work

Complete Phase 1 by reviewing its data model, parameter schema, callback and error/lifetime contracts and process-wide GSL policy with IOC integrators. Review distribution obligations for the actual IOC and its dependencies. Phase 2 begins with the explicitly requested aSub mean prototype; the IOC-wide handler policy remains a documented integration review item. Phase 3 covers FFT and filtering; Phase 4 evaluates dedicated records; Phase 5 expands GSL coverage; Phase 6 focuses on measured performance and portability. None of those interfaces is implemented yet.

## Phase 2 increment: aSub mean prototype

- Implemented: synchronous DOUBLE waveform-to-scalar mean, explicit IOC initialization, alarm handling, an example database and IOC tests. Files include `numericApp/src/numericAsub.cpp`, `numericSupport.dbd`, the IOC test and `examples/mean.db`; CI runs both initialization cases.
- Local validation: `make GSL_DIR=/tmp/numeric-gsl-install`, `numericTest`, and initialized/uninitialized `numericIocTest` passed on macOS arm64 using GSL 2.8 and EPICS Base R7.0.10-151-g0cc912b17-dirty. Linux CI passed on commit `195db3c`. Commands are in README.
- [x] Verify normal, short, invalid and recovery input, alarms, output preservation, explicit processing, and missing initialization through an IOC.
- [x] Verify clean EPICS Base 7.0.10 Linux CI for the aSub prototype.
- [ ] Review the IOC-wide GSL error policy.
- [ ] Complete Phase 2 streaming statistics, reset/sample-count semantics and tests; the current prototype has no streaming state.

Next: review this narrow aSub interface and complete Phase 2 in separately tested increments.

## Running mean core increment

- `running_mean` owns a GSL running-statistics workspace, returns a running mean and sample count, and supports reset. Native tests cover first sample, repeated samples, independent instances, invalid input and reset. See [streaming](streaming.md).
- Local build and native test passed on macOS arm64 with GSL 2.8 and the supplied EPICS Base development checkout. Linux CI passed for the core increment.
- [x] Implement and test core running mean, ownership, validation, sample counts and reset.
- [x] Define an explicitly sequenced aSub streaming prototype with reset and IOC tests; see [streaming](streaming.md).
- [ ] Verify Linux CI for the aSub streaming prototype and expand its lifecycle and integration coverage.
- [ ] Implement further Phase 2 statistics and a lossless, coordinated sample transport where required.
