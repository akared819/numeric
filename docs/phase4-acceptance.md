# Phase 4 acceptance: dedicated records

**Status: complete for the general-purpose dedicated record scope**, subject to the limits below. `numericScalar` and `numericArray` expose a defined subset of the registry through native EPICS record support. This does not claim every GSL function, deployment-specific IOC integration or a hard real-time guarantee. The acceptance PR must pass required Linux CI checks before merge.

| Criterion | Evidence |
| --- | --- |
| A dedicated scalar-result record is available. | `numericScalar` uses compatible stateless single-input scalar-result operations; [scalar record](numeric-scalar.md) documents fields, inputs, processing and errors. Native IOC tests verify mean and sample variance, invalid inputs, truncation, alarm and recovery, monitor, timestamp, forward link and read-only value behavior. |
| A dedicated array-result record is available. | `numericArray` supports the documented FFT, moving mean/median, Gaussian and uniform histogram operations; [array record](numeric-array.md) defines fields, buffers, parameters and publication. IOC tests verify numerical results, input and output capacity, invalid inputs, output preservation, monitors, timestamp, forward link and write protection. |
| The records integrate with an IOC and separate clients. | The standalone example loads both record databases. Its native Channel Access clients calculate through both records, verify an array subscription established before the first result and check that the array result rejects writes. |
| Resource ownership and error handling are explicit. | Each record owns its operation and buffers. EPICS exit callbacks release them when `epicsExit` or `epicsExitCallAtExits` is invoked; IOC tests shut down, clean up the test database and exercise the exit callbacks. GSL's process-wide non-terminating handler policy is described in [architecture](architecture.md#gsl-setup-for-an-ioc). |
| Validation and coverage are reproducible. | [README](../README.md) lists build and test commands; CI executes native, record IOC, example and regression suites on Linux. [GSL coverage](gsl-coverage.md) identifies the operations exposed. Local and final Linux results are recorded in the acceptance PR. |

## Remaining boundaries

- `numericScalar` accepts only nonconstant DOUBLE inputs and stateless operations with exactly one scalar or array input and one scalar output. `numericArray` supports only its documented array-result operations; OP and buffer capacities are fixed at database load. The records have no output link or acknowledged delivery, and input updates do not automatically process them.
- Record processing is synchronous. Real FFT and window operations can allocate workspace on first use or on size changes. Assess timing and resource limits for each IOC; no hard real-time suitability is claimed.
- Per-record resources are freed when EPICS exit callbacks run, not at `iocShutdown`. Dynamic database deletion/recreation in a live process is unsupported. Returning from `main` or terminating via a signal does not run EPICS exit callbacks by itself; the OS reclaims process memory at termination.
- Operations requiring multiple waveform inputs or multiple result arrays, stateful streaming, complex parameter sets and solvers are not exposed by these records. Existing aSub interfaces remain available, with their own documented behavior.
- All GSL users in an IOC must agree on the non-terminating error-handler policy. The intermittent EPICS Base 7.0.10 same-process put-notify teardown crash remains unresolved; it has been reproduced without numeric or GSL linked, and its control remains a required CI check. See [Phase 2 acceptance](phase2-acceptance.md).
