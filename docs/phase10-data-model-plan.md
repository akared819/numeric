# Phase 10: typed array model — in progress

The existing Float64 `Input`, `Output` and operation APIs remain supported.
The separate `numericData.h` interface provides typed views, exact same-type
element reads and writes, and explicit exact element conversion independently
of GSL. The separate `TypedMean` adapter connects these views to GSL's
[`gsl_stats_mean`](https://www.gnu.org/software/gsl/doc/html/statistics.html#mean-standard-deviation-and-variance).
It is a native C++ API and is not yet exposed through EPICS records.

The supported view types are Float32, Float64, Int32, Int64, Complex64 and
Complex128. Complex elements use two consecutive real components
(`real`, `imag`), with 32-bit components for Complex64 and 64-bit components
for Complex128. Components use native byte order. The build requires IEEE
binary32 and binary64. This is an in-process representation, not a network
byte format. Access uses byte copies rather than assuming alignment or a
particular `std::complex` representation.

Views have nonnegative byte strides and at most eight dimensions. Rank zero is
a scalar. Any zero extent is an empty view and may have a null data pointer;
no element can be accessed. For nonempty views, validation checks capacity,
stride and offset overflow. Repeated dimensions require nonzero stride.
Read-only views may overlap; writable views use a conservative stride-order
check to reject overlap and some non-overlapping layouts it cannot prove safe.
Element access uses `memcpy` so storage need not be aligned. Reads and writes
reject non-finite elements; same-type writes require exact representation and
leave the destination unchanged on error. Caller-owned buffers and dimension
arrays must outlive each operation invocation and must not be modified
concurrently with validation or access.

`convertElement` rejects integer-to-floating conversions that are not exactly
representable, and checks finiteness, fractional values and range before
floating-to-integer conversion. Complex-to-real conversion requires an exactly
zero imaginary component. All conversions are exact; nonzero imaginary values
and lost precision are rejected. The destination value is unchanged on error.

`TypedMean` accepts a nonempty view, flattens its elements in row-major index
order and converts each exactly to Float64 before calling GSL. Complex values
must have zero imaginary part, and Int64 values must be exactly representable
as Float64. A caller-provided maximum element count bounds its per-instance
scratch buffer. `reserve()` may allocate in advance; repeated calculations
within that capacity reuse it, and `reset()` releases it. Concurrent calls on
one instance require external synchronization. Invalid inputs and non-finite
results preserve the output; `numericInitialize`/`initializeGsl` must run
before calculation. This implementation validates the view once, then copies
and validates its elements. [Benchmarks](benchmarks.md#typed-view-comparison)
show the cost of this path relative to the existing Float64 mean.

The native `numericDataTest` covers all six types, complex layout and unaligned
access, large Int64 values, empty views, invalid indices, capacity, overflow,
overlapping writes and preservation on failure. `numericTypedMeanTest` checks
GSL calculations across all six types, a strided matrix, exact-conversion
failures, initialization, capacity and reset. Acceptance also requires
additional numerical operations, EPICS integration, all platform CI, and
performance comparisons. The model must
not claim NumPy compatibility or support arbitrary multidimensional layouts
before those cases are verified.
