# Complex radix-2 FFT core operations

The registry exposes `fft_complex_radix2_forward` and `fft_complex_radix2_inverse`. Both take one FLOAT64 array input and one FLOAT64 array output. Each complex number is represented by consecutive doubles `[real, imaginary]`, in natural sample or frequency-bin order. Counts and capacities are numbers of doubles, not complex numbers. Input stride must be 1. The input count must be even and represent a positive power-of-two number of complex values (including one); both buffers must have capacity for the full count. The output count is that same number of doubles on success and zero on failure.

Forward uses the negative exponential, without normalization. Inverse uses the positive exponential and divides by the complex length, so a forward/inverse pair returns the original values within floating-point error. These conventions match the [GSL FFT documentation](https://www.gnu.org/software/gsl/doc/html/fft.html). Neither operation computes magnitudes, frequencies, windowing, or real/half-complex transforms.

Buffers can be identical for in-place operation or disjoint; partial overlap is rejected. Validation rejects null buffers, non-array types, non-unit strides, invalid lengths or capacities, and NaN or infinity. Input validation failures leave the data unchanged. On GSL failure or a non-finite numerical result, output count is zero and **output contents are unspecified**: the in-place transform may already have changed them. An in-place caller that must retain its input on numerical failure must use a separate output buffer. Floating-point underflow and rounding are not separately diagnosed.

The operation objects contain only a direction flag; GSL's radix-2 routines require no additional workspace and the execution path allocates no heap memory. Each caller owns its input and output buffers; they must remain valid and must not be concurrently modified for the duration of execution. Objects have no mutable computational state, but synchronization of shared buffers is the caller's responsibility. Call `numeric::initializeGsl()` at single-threaded startup as described in [architecture](architecture.md#gsl-setup-for-an-ioc). The operations are also exposed through the aSub interface described below.

Build and test from the repository root using the paths in the [README](../README.md):

```sh
make GSL_DIR=/absolute/path/to/gsl
./bin/$(/absolute/path/to/epics-base/startup/EpicsHostArch)/numericFftTest
```

## EPICS aSub interface

`numericFftForward` and `numericFftInverse` expose the corresponding core operations through an aSub record. [fft.db](../examples/fft.db) supplies a four-complex-sample forward and inverse example. Configure `FTA` and `FTVA` as DOUBLE, `INPA` as a non-constant input link, `NOA` strictly greater than the largest valid input count, and `NOVA` at least that count. The extra input element lets the routine detect a possibly truncated input: it rejects `NEA >= NOA`, including an exact fit. `NEA`, `NOVA` and `NEVA` count doubles; the complex count is half of `NEA`. The example has an eight-double waveform and sets `NOA` to nine.

When `OUTA` is configured, its target must report DOUBLE type and capacity for at least `NEA` doubles. If its type or capacity cannot be determined, including when a remote link is disconnected, processing fails rather than knowingly publishing a truncated result. An unconnected constant output link is permitted. EPICS performs the actual link write after the routine returns; its success and delivery are subject to EPICS link semantics, and the capacity can change independently after validation. Consumers must monitor their links and status; this interface does not provide delivery acknowledgment.

The record owns the A and VALA buffers. Each process fetches A, transforms that record-owned buffer in place, and copies the complete result to VALA only on success; A therefore contains scratch data after processing and must not be treated as the original input. No allocation is performed during the calculation. The source waveform is not modified. A constant INPA is rejected because it is not refreshed on each process. Setting an input waveform does not automatically process this aSub; arrange processing explicitly. For the example, process `fftForward` after updating `fftInput`, then process `fftInverse` to reconstruct the signal from `fftOutput`.

On success, `VAL` is zero, `NEVA` equals the number of doubles and aSub writes OUTA. A bad configuration, invalid length or values, uninitialized GSL policy, or non-finite result sets CALC/INVALID, clears `NEVA`, returns -1 and leaves VALA and the downstream output untouched. Record support does not write output links on a nonzero subroutine return. A failed input-link fetch occurs before the routine and follows EPICS record behavior; inspect alarm and link status in addition to `VAL`. See the [EPICS aSub documentation](https://docs.epics-controls.org/projects/base/en/latest/aSubRecord.html).
