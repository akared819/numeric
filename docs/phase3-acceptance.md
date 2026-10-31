# Phase 3 acceptance: general-purpose module

**Status: complete for the general-purpose batch interfaces and bundled example IOC**, subject to the documented limitations below. Phase 3 establishes a usable FFT, filtering and moving-statistics foundation; it does not claim complete GSL coverage or facility-specific IOC integration. The corresponding PR must pass the required Linux CI checks before merging.

| Criterion | Evidence |
| --- | --- |
| GSL FFTs are available through the native registry and EPICS waveform records. | Complex radix-2 forward and inverse, and real-input radix-2 forward operations are documented in [FFT](fft.md). Native tests cover impulse, sine, cosine, sizes, round trips, invalid buffers and numerical failure; IOC tests cover capacities, alarms, output preservation and recovery. |
| A GSL filter and moving statistics are available through the native registry and EPICS. | Gaussian smoothing, moving mean and moving median are documented in [window statistics](window-statistics.md). Native and IOC tests cover analytical results, endpoints, window changes, invalid inputs and independent workspaces. |
| Numerical state and memory ownership are defined. | FFT scratch and window workspaces are per-instance, released by reset/destruction or record cleanup, with allocation and concurrency behavior documented for each interface. GSL's process-wide non-terminating error policy is documented in [architecture](architecture.md#gsl-setup-for-an-ioc). |
| A standalone example can be exercised by a separate client. | `examples/ioc/st.cmd` loads the FFT and window databases. `tests/example-ioc.sh` runs native Channel Access clients against it, covering both FFT layouts, complex inverse, moving mean, moving median and Gaussian smoothing. |
| Verification is reproducible. | The [README](../README.md) records build and test commands. Linux CI runs native and IOC tests, the example, and existing regression checks. The local macOS arm64 build and the seven Phase 2 plus six Phase 3 example assertions passed in this increment. |
| Coverage and limits are explicit. | [GSL coverage](gsl-coverage.md) lists accessible operations without claiming the remaining GSL modules. The operation guides define buffer layout, endpoint rules, parameter bounds, errors and output-link behavior. |

## Remaining boundaries

- FFT support is limited to radix-2 transforms: complex forward/inverse and real forward returning the full complex spectrum. Direct real inverse, mixed-radix transforms, window functions, PSD calculations and wavelets remain future work. The complex inverse can reconstruct real input from a full spectrum within numerical error.
- Window operations are centered batch calculations with endpoint-value padding and documented parameter bounds. Other statistics, endpoint modes, causal or streaming filters, and a guarantee that every PV update is processed are not provided.
- Real FFT scratch and window workspaces may allocate on first use or when sizes change. Execution is synchronous; no hard real-time or performance guarantee is made. Integrators must assess workload and IOC scheduling for their deployment.
- Output links can fail after validation and do not provide acknowledged delivery. Sources and configuration parameters must be coordinated by the producer. Follow the [FFT](fft.md) and [window statistics](window-statistics.md) guides for buffer and link requirements.
- All GSL users in an IOC must agree on a non-terminating process-wide error-handler policy. Numeric cannot prevent another component from replacing it.
- An intermittent same-process CA put-notify teardown crash has been reproduced in an EPICS Base 7.0.10-only control without numeric or GSL linked. Its exact cause remains unconfirmed. The stress control remains a required CI check; a passing run does not prove the crash is fixed. See [Phase 2 acceptance](phase2-acceptance.md) for the evidence and scope decision.
