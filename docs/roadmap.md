# Roadmap and acceptance

The general-purpose Phase 2 target and its acceptance evidence are summarized in [Phase 2 acceptance](phase2-acceptance.md). The increment sections below record work and open items as they stood at each step; later sections may supersede earlier unchecked items.

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
- [x] Linux CI passed for the aSub streaming prototype (PR #4).
- [ ] Expand streaming lifecycle and integration coverage.
- [ ] Implement further Phase 2 statistics and a lossless, coordinated sample transport where required.

## Phase 2 increment: batch sample variance

- Added GSL sample variance to the registry and the `numericVariance` aSub adapter, with native and IOC tests and an example database.
- [x] Test numerical correctness, invalid sizes, non-finite values, waveform capacity, alarms and recovery locally.
- [x] Linux CI passed for this increment (PR #5).

## Phase 2 increment: standard deviation and running statistics

- Added batch sample standard deviation and population variance and standard deviation; grouped running mean, variance, standard deviation, RMS, minimum, maximum and sample count, with aSub interfaces and native/IOC tests.
- [x] macOS arm64: build, native core test, 154 IOC assertions and 22 uninitialized assertions passed using GSL 2.8 and the supplied EPICS Base development checkout.
- [x] Linux CI passed for this increment (PR #6).
- [x] Validate and review the acknowledged streaming protocol described below (PR #7).
- [ ] Complete remaining Phase 2 acceptance decisions, including the IOC-wide GSL policy for each deployment.

## Phase 2 increment: acknowledged streaming

- Added a single-producer protocol with an atomic event waveform, sequence checks, duplicate replay, ordered reset and explicit acknowledgment; see [streaming transport](streaming-transport.md).
- [x] Local macOS arm64 build, native core tests, 194 IOC assertions, 25 missing-initialization assertions and eight loopback Channel Access assertions passed using GSL 2.8 and the supplied EPICS Base checkout. The CA test destroys and recreates client channels; it does not simulate network partition or IOC crash.
- [x] Linux CI passed and PR #7 was reviewed and merged. The producer must obey one-in-flight backpressure; the IOC cannot enforce exclusive access to the waveform slot. Durable recovery, arbitrary concurrent producers, and server-enforced buffering are not implemented.

## Phase 2 increment: native acknowledged sender

- Added `numericAckSend` with bounded retries, acknowledgment checks and explicit uncertain outcomes; see [streaming transport](streaming-transport.md).
- [x] macOS arm64 build and native tests, 194 initialized and 25 uninitialized IOC assertions, and 28 loopback CA assertions passed with GSL 2.8 and the supplied EPICS Base development checkout.
- [x] Linux CI passed for the sender increment (PR #8).
- [ ] Deployment-specific integration and IOC-wide GSL handler agreement remain outstanding. The sender does not implement durable recovery or multiple-producer arbitration.

## Phase 2 increment: separate-process CA timeout test

- Added a test IOC with a deliberately slow synchronous processing chain and a separate CA client. The client disconnects after a timeout, checks the IOC completed the original event, retries it, and verifies the count and next sample.
- [x] macOS arm64: native build and all five separate-process CA assertions passed with the supplied EPICS Base development checkout and GSL 2.8.
- [x] Linux CI passed after replacing the sender and same-process test helper's put-notify calls with plain puts (PR #9). The underlying intermittent EPICS notification lifetime cause remains unconfirmed; network tests continue to exercise put callbacks.
- [ ] Test actual network partitions and target IOC integration. The test does not simulate an IOC crash, multiple producers, or durable recovery.

## Phase 2 increment: IOC restart validation

- Added a separate-process test of stream state after restarting the test IOC: old nonzero sequences are not acknowledged, a new session starts empty, and a reused first event demonstrates the lack of durable identity.
- [x] macOS arm64: build, native core tests, 194 initialized and 25 uninitialized IOC assertions, 28 same-process CA assertions, and 39 separate-process CA assertions passed with GSL 2.8 and the supplied EPICS Base development checkout.
- [x] Linux CI passed the restart test and existing regressions on the initial PR #10 commit. A clean process restart is not a crash during processing, and the sender does not detect restarts or reconstruct state.

## Phase 2 increment: controlled IOC crash

- Kill the separate test IOC after a test-only delay signals that event processing has begun, before the numerical consumer runs. Check that the sender reports uncertainty and a fresh session can start after restart.
- [x] macOS arm64: build, native core tests, 194 initialized and 25 uninitialized IOC assertions, and 28 same-process CA assertions passed using GSL 2.8 and the supplied EPICS Base development checkout.
- [x] The corrected crash setup and all 44 separate-process CA assertions passed on macOS arm64.
- [x] Linux CI passed on the corrected PR #11 commit. That test covered a crash before numerical state changes.
- A network partition and durable recovery remain unimplemented. Deployment-specific restart detection and recovery remain the integrator's responsibility.

## Phase 2 increment: crash after numerical update

- The test-only IOC verifies the coherent result after the consumer runs and pauses the same processing chain. The test kills the IOC before the sender obtains the acknowledgment and checks that the sender reports uncertainty. After restart, it demonstrates that the old first event can be accepted again and verifies a separate fresh session starts empty.
- [x] macOS arm64: native build and core test, 194 initialized and 25 uninitialized IOC assertions, 28 same-process CA assertions, and 47 separate-process CA assertions passed using GSL 2.8 and the supplied EPICS Base checkout. Linux CI results are tracked in the pull request.
- This tests one controlled crash point, not all failure timings, a network partition, or durable recovery.

## Phase 2 increment: batch minimum and maximum

- Add `minmax` to the registry and `numericMinmax` aSub support, using GSL's single-pass routine. It validates the array and both outputs before calculation; the stateless instance owns no workspace.
- Acceptance: native numerical and invalid-input tests; IOC tests for initialization, empty and truncated input, two outputs, alarms and recovery; Linux CI. The example and public contract are described in [batch statistics](asub-mean.md).
- [x] macOS arm64 with GSL 2.8 and the supplied EPICS Base development checkout: build and native core test, 223 initialized and 28 uninitialized IOC assertions, 28 same-process CA assertions, and 47 separate-process CA assertions passed. Linux CI results are tracked in the pull request.
- A network partition test, deployment-specific recovery and IOC-wide GSL handler agreement remain open.

## Phase 2 increment: batch sample covariance

- Add `covariance` and `numericCovariance`, using GSL for sample covariance on two equal-length finite DOUBLE arrays. Each input is checked independently for capacity and truncation; the stateless operation owns no workspace.
- Acceptance: native numerical, stride, dimension and invalid-input tests; IOC tests for both inputs, truncation, alarms, recovery and the example database; Linux CI. Inputs are fetched separately and require producer coordination as documented in [batch statistics](asub-mean.md).
- [x] macOS arm64 with GSL 2.8 and the supplied EPICS Base development checkout: build and native core test, 260 initialized and 31 uninitialized IOC assertions, 28 same-process CA assertions, and 47 separate-process CA assertions passed. Linux CI results are tracked in the pull request.
- A network partition test, deployment-specific recovery and IOC-wide GSL handler agreement remain open.

## Phase 2 increment: paired waveform covariance

- Add `numericCovariancePairs` for a single interleaved DOUBLE waveform, reusing the sample covariance operation through two strided views. It requires complete finite pairs, validates aSub capacity, and does not allocate or copy during calculation.
- Acceptance: verify the existing core with two views of one buffer; IOC tests for normal, empty, odd, short, non-finite and full inputs, truncated aSub buffers, alarms, recovery and the example; Linux CI. The producer must publish complete paired updates and arrange processing as described in [batch statistics](asub-mean.md).
- [x] macOS arm64 with GSL 2.8 and the supplied EPICS Base development checkout: build and native core test, 303 initialized and 34 uninitialized IOC assertions, 28 same-process CA assertions, and 47 separate-process CA assertions passed. Linux CI results are tracked in the pull request.
- A network partition test, deployment-specific recovery and IOC-wide GSL handler agreement remain open.

## Phase 2 increment: Pearson correlation

- Add `correlation`, `numericCorrelation`, and `numericCorrelationPairs` using GSL's Pearson correlation. Both input layouts share the covariance buffer validation, with extra checks for constant inputs and conservatively unsafe numerical ranges.
- Acceptance: native tests for positive and negative correlation, constant, non-finite, invalid and unsafe inputs; IOC tests of both layouts, truncation, alarms, recovery and the example; Linux CI. See [batch statistics](asub-mean.md) for range and input-coordination limits.
- [x] macOS arm64 with GSL 2.8 and the supplied EPICS Base development checkout: build and native core test, 360 initialized and 40 uninitialized IOC assertions, 28 same-process CA assertions, and 47 separate-process CA assertions passed. Linux CI results are tracked in the pull request.
- A network partition test, deployment-specific recovery and IOC-wide GSL handler agreement remain open.

## Phase 2 increment: controlled connection interruption

- Add a test-only CA loopback relay. Cut the sender's connection after the IOC receives the event, verify the still-running IOC accepted it through an independent client, verify a new event cannot be acknowledged while disconnected, then restore the relay and verify retry and subsequent sequence behavior.
- Acceptance: native relay and test client build, controlled disconnection and replay tests pass locally and repeatedly on Linux CI; document the fault model and remaining limits.
- [x] macOS arm64 with GSL 2.8 and the supplied EPICS Base development checkout: build and core test, 360 initialized and 40 uninitialized IOC assertions, 28 same-process and 47 separate-process CA assertions passed. The relay interruption test passed, including its disconnected negative control; Linux CI results are tracked in the pull request.
- This test covers one connection failure, not arbitrary partitions or durable recovery. Deployment-specific recovery and IOC-wide GSL handler agreement remain open.

## Phase 2 increment: standalone example IOC and acceptance

- Add a runnable `numericExample` IOC, startup script, and native CA smoke test for batch mean, paired covariance, acknowledged streaming and reset. The mean example reserves an extra input element so a full source waveform is accepted without ambiguous truncation.
- [x] macOS arm64 with GSL 2.8 and the supplied EPICS Base development checkout: build and native core test, 360 initialized and 40 uninitialized IOC assertions, 28 same-process and 47 separate-process CA assertions, the connection-interruption test, and two runs of the seven-assertion standalone example IOC test passed. Linux CI results are tracked in the pull request.
- [Phase 2 acceptance](phase2-acceptance.md) records the general-purpose scope, checks and remaining deployment-specific limitations.
