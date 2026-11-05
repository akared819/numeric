# Complex radix-2 FFT core operations

The registry also exposes `fft_complex_mixed_forward` and
`fft_complex_mixed_inverse`, with the same interleaved complex layout and
normalization as the radix-2 operations. Unlike radix-2, the complex length
may be any positive integer. GSL's general-factor algorithm can be slower for
large prime factors. Each instance owns and reuses a GSL wavetable, workspace
and scratch buffer; changing length may allocate, while reset and destruction
release these resources. Input and output may overlap; failures preserve the
output values and set the output count to zero. Do not use one instance from
multiple threads without synchronization. Both operations are also available
through numericArray. See the [GSL FFT documentation](https://www.gnu.org/software/gsl/doc/html/fft.html).

`fft_frequency_axis` generates N signed frequency-bin centers from two scalar
inputs: a positive integer sample count N and a positive sample interval in
seconds. The result is in hertz and follows the FFT's natural order: zero,
positive frequencies, then negative frequencies. For even N, the Nyquist bin
is negative. For example, N=4 and interval=0.5 produce `[0, 0.5, -1, -0.5]`.
The operation checks the output capacity and floating-point range and does not
allocate; invalid input leaves output values unchanged. It is an arithmetic
utility, not a GSL FFT call, and is available through numericArray.

`window_hann` multiplies an input waveform by the symmetric Hann window
`0.5 * (1 - cos(2*pi*i/(N-1)))`; for N=1 the input is unchanged. It is a
high-level arithmetic operation rather than a direct GSL wrapper. Its
per-instance scratch buffer allows overlapping input and output and is reused
until reset or destruction. Invalid inputs leave output values unchanged.
No amplitude or power compensation is applied. It is available through
numericArray.

`spectrum_power` converts a full interleaved complex spectrum to one real
value per bin, `real² + imaginary²`. The result is raw squared magnitude: no
FFT-length, window, sampling-rate, or one-sided correction is applied, so it
is not a power spectral density. Its scratch buffer is reused and released on
reset or destruction. It is a high-level arithmetic operation, exposed through
the native registry and numericArray; invalid input or overflow preserves the
output.

`welch_psd` calculates a one-sided power spectral density by averaging
overlapping, symmetric-Hann-windowed segments using GSL radix-2 real FFTs.
The four inputs are the real waveform, segment length N, overlap in samples,
and sample interval in seconds. N must be a power of two of at least four,
overlap must be an integer in `[0, N)`, and the waveform must contain at least
N samples. The final incomplete segment is ignored and no detrending is done.
The N/2+1 output bins cover zero through Nyquist with spacing `1/(N*dt)` Hz.
Each segment's squared FFT is scaled by `dt/sum(window²)`, with interior
positive-frequency bins doubled; output units are input-unit²/Hz. This is a
composition around a GSL FFT, not a direct GSL PSD routine. The vectors are
owned by the operation and reused; reset or destruction releases them. Invalid
inputs, allocation failures and non-finite results preserve output values.
numericArray supports the four inputs through INPA–INPD; updates to separate
links are not an atomic snapshot.

`stft_hann` accepts a real waveform, frame length N, and hop size in samples.
N must be a power of two of at least four; hop must be an integer in `[1, N]`.
It applies the symmetric Hann window to each complete frame and uses GSL's
radix-2 real FFT. No detrending, scaling, padding, or incomplete final frame
is included. The output is a flattened sequence of frames: each frame contains
N complex bins in natural frequency order as 2N interleaved real/imaginary
doubles. The frame count is `1 + floor((sample_count-N)/hop)`, and output
capacity must hold `2*N*frame_count` doubles. The instance owns and reuses its
work buffers, released by reset or destruction. Invalid data, allocation or
numerical failure preserves output values. numericArray exposes waveform,
frame length and hop through INPA–INPC; separately updated links are not an
atomic snapshot. This is a composition using GSL FFT primitives.

The example IOC loads [spectral-analysis.db](../examples/spectral-analysis.db)
with 16 samples, eight-sample frames, four-sample overlap/hop and a 0.01-second
sample interval. For example, after starting the IOC:

```sh
caput -a demo:SPECTRAL:INPUT 16 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1
caput demo:SPECTRAL:PSD.PROC 1
caget -a demo:SPECTRAL:PSD.VAL
caput demo:SPECTRAL:STFT.PROC 1
caget -a demo:SPECTRAL:STFT.VAL
```

Changing an input or setting alone does not trigger processing. After changing
frame length, overlap or hop, ensure the configured output capacity is large
enough. Check the result severity before using values: an INVALID alarm can
leave the previous valid output visible.

The registry exposes `fft_complex_radix2_forward` and `fft_complex_radix2_inverse`. Both take one FLOAT64 array input and one FLOAT64 array output. Each complex number is represented by consecutive doubles `[real, imaginary]`, in natural sample or frequency-bin order. Counts and capacities are numbers of doubles, not complex numbers. Input stride must be 1. The input count must be even and represent a positive power-of-two number of complex values (including one); both buffers must have capacity for the full count. The output count is that same number of doubles on success and zero on failure.

Forward uses the negative exponential, without normalization. Inverse uses the positive exponential and divides by the complex length, so a forward/inverse pair returns the original values within floating-point error. These conventions match the [GSL FFT documentation](https://www.gnu.org/software/gsl/doc/html/fft.html). Neither complex operation computes magnitudes, frequencies, or windowing. A separate real-input operation is described below.

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

## Real-input radix-2 FFT

The native registry operation `fft_real_radix2_forward` takes one nonempty contiguous FLOAT64 array of N real samples, where N is a power of two (including one). It returns a full N-bin complex spectrum as 2N interleaved DOUBLE values in the same natural order and forward-sign convention as `fft_complex_radix2_forward`. Thus a real impulse `[1,0,0,0]` returns `[1,0,1,0,1,0,1,0]`. The output is Hermitian; both positive and negative frequency bins are present. Use the complex inverse operation to reconstruct a real signal, allowing for numerical error in the imaginary parts. This operation calls GSL's `gsl_fft_real_radix2_transform` and `gsl_fft_halfcomplex_radix2_unpack` rather than exposing GSL's packed half-complex layout.

Output capacity must be at least 2N doubles. Input and output may overlap because the input is copied into a per-instance scratch vector before output is written. The instance allocates or resizes that vector as needed and reuses its capacity; `reset()` and destruction release it. Do not call the same instance concurrently. Finite inputs, lengths, types, stride and capacities are validated; output count is zero on failure. Validation and real-transform failures leave output unchanged; if unpacking itself fails, output contents are unspecified. Check the result before using it. The output count is 2N on success. No magnitude, power normalization or window function is applied. Mixed-radix real FFTs are not yet exposed.

`fft_real_radix2_inverse` accepts this full interleaved spectrum and returns N
real samples using GSL's half-complex radix-2 inverse, scaled by 1/N. N must be
a positive power of two and the output must hold N doubles. The input must be
finite and exactly conjugate symmetric, including real DC and Nyquist bins;
approximate symmetry is rejected rather than silently discarding a component.
The per-instance scratch buffer is reused and released by reset or destruction.
Input and output may overlap; errors leave output values unchanged and set the
output count to zero. The operation is also available through numericArray.
See [GSL FFT documentation](https://www.gnu.org/software/gsl/doc/html/fft.html).

`numericRealFftForward` exposes this operation through an aSub record; see [fft.db](../examples/fft.db). Set FTA, FTVA and FTVB to DOUBLE, use a nonconstant INPA, set NOA strictly greater than the maximum input count, and reserve at least 2N elements in both NOVA and NOVB. VALB is record-owned staging space, is not a result field, and OUTB must remain an unconfigured constant link. The operation also owns an N-double scratch vector per record, allocated on first use or when necessary and released through the record cleanup callback. A is not modified. On success, the spectrum is copied from VALB to VALA, NEVA is 2N and NEVB is zero. The OUTA target, if connected, must report DOUBLE type and capacity of at least 2N; actual EPICS link delivery is not acknowledged. Invalid input, insufficient buffers or output capacity, numerical failure or missing initialization sets CALC/INVALID, clears NEVA and NEVB and leaves VALA and the downstream output unchanged. An input fetch failure occurs before the routine and follows EPICS record behavior. Updating the input does not itself trigger processing.
