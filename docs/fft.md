# Complex radix-2 FFT core operations

The registry exposes `fft_complex_radix2_forward` and `fft_complex_radix2_inverse`. Both take one FLOAT64 array input and one FLOAT64 array output. Each complex number is represented by consecutive doubles `[real, imaginary]`, in natural sample or frequency-bin order. Counts and capacities are numbers of doubles, not complex numbers. Input stride must be 1. The input count must be even and represent a positive power-of-two number of complex values (including one); both buffers must have capacity for the full count. The output count is that same number of doubles on success and zero on failure.

Forward uses the negative exponential, without normalization. Inverse uses the positive exponential and divides by the complex length, so a forward/inverse pair returns the original values within floating-point error. These conventions match the [GSL FFT documentation](https://www.gnu.org/software/gsl/doc/html/fft.html). Neither operation computes magnitudes, frequencies, windowing, or real/half-complex transforms.

Buffers can be identical for in-place operation or disjoint; partial overlap is rejected. Validation rejects null buffers, non-array types, non-unit strides, invalid lengths or capacities, and NaN or infinity. Input validation failures leave the data unchanged. On GSL failure or a non-finite numerical result, output count is zero and **output contents are unspecified**: the in-place transform may already have changed them. An in-place caller that must retain its input on numerical failure must use a separate output buffer. Floating-point underflow and rounding are not separately diagnosed.

The operation objects contain only a direction flag; GSL's radix-2 routines require no additional workspace and the execution path allocates no heap memory. Each caller owns its input and output buffers; they must remain valid and must not be concurrently modified for the duration of execution. Objects have no mutable computational state, but synchronization of shared buffers is the caller's responsibility. Call `numeric::initializeGsl()` at single-threaded startup as described in [architecture](architecture.md#gsl-setup-for-an-ioc). These operations are currently available through the native registry only; no EPICS FFT record interface is provided yet.

Build and test from the repository root using the paths in the [README](../README.md):

```sh
make GSL_DIR=/absolute/path/to/gsl
./bin/$(/absolute/path/to/epics-base/startup/EpicsHostArch)/numericFftTest
```
