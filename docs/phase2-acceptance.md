# Phase 2 acceptance: general-purpose module

**Status: complete for the general-purpose module and bundled example IOC.** This scope does not include integration into a particular facility's IOC or a guarantee of durable delivery. The criteria below are met by the implementation and tests; Linux CI passed on the implementation commit in [PR #18](https://github.com/akared819/numeric/pull/18). The boundaries listed below remain in effect.

| Criterion | Evidence |
| --- | --- |
| Batch statistics are available through the native registry and EPICS aSub records. | Mean, minimum/maximum, sample and population variance and standard deviation, sample covariance, and Pearson correlation are documented in [batch statistics](asub-mean.md) and tested by `numericTest` and `numericIocTest`. |
| Streaming statistics have per-instance state, defined first-sample behavior, counting and reset. | `running_mean` and `running_stats` use owned GSL workspaces. Numerical and IOC tests cover samples, independent instances, invalid input and reset; conventions are documented in [streaming](streaming.md). |
| The example supports a coordinated stream with explicit identity and acknowledgment. | The single-producer protocol, retry, reset and output fields are documented in [streaming transport](streaming-transport.md). Native, IOC and CA tests cover sequencing, duplicate replay and invalid input. |
| Errors do not intentionally terminate the IOC and are exposed through status or alarms. | Startup calls `numericInitialize` before `iocInit`; invalid input and uninitialized IOC tests check failure behavior. The process-wide GSL handler agreement remains an integration requirement, as documented in [architecture](architecture.md#gsl-setup-for-an-ioc). |
| A standalone example IOC can be built, started and exercised by a separate native client. | `examples/ioc/st.cmd` loads batch and streaming examples. `tests/example-ioc.sh` verifies a batch waveform, paired covariance, acknowledged samples and reset. |
| Regression checks execute on supported Linux and locally where available. | Linux CI runs the native, IOC, same-process CA, separate-process CA, interruption and example IOC tests. Build and test commands are in the [README](../README.md). |

## Boundaries that remain

- The acknowledged interface is a cooperative, single-producer protocol for a running session. It is not a durable queue, does not arbitrate independent producers, and does not guarantee every PV update is processed. The caller must preserve event identity, wait for acknowledgment and use a new session after a restart.
- IOC state and acknowledgments are lost on restart. A connection interruption after receipt, a timeout, and two controlled crash points are tested; arbitrary network partitions, every possible failure timing and durable recovery are not covered. A facility must define its own restart detection and reconciliation procedure.
- The IOC integrator must confirm that all GSL users in the process agree on a non-terminating error-handler policy. Numeric cannot prevent another component from replacing it. The documented upstream GSL workspace allocation-failure caveat remains.
- The module supports DOUBLE inputs in these interfaces. Input links and output links have the acquisition, truncation and atomicity limits documented with each operation. No hard real-time guarantee is made.
- FFT, filtering, moving statistics, dedicated record types and broader GSL coverage are subsequent work; they are not implied by closing this Phase 2 scope.
