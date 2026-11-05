# Phase 10: typed array model — proposed design

This design is not implemented yet. The current Float64 `Input`, `Output` and
operation APIs remain supported. New generic views will be separate from GSL
adapters and will identify the element type, rank, extents, byte strides,
buffer capacity and read/write ownership without exposing GSL workspaces.

The initial target types are Float32, Float64, Int32, Int64, Complex64 and
Complex128. Complex elements will use two consecutive real components
(`real`, `imag`), with 32-bit components for Complex64 and 64-bit components
for Complex128. Access will use byte copies rather than assuming alignment or
that C++ `std::complex` has a particular representation. Layout details,
endianness and any platform requirements must be stated with the implemented
API.

Views will initially have nonnegative byte strides. Validation must check rank,
extent, stride, buffer bounds and all products and offsets for overflow. A
writeable view must not permit ambiguous overlapping elements; if the first
implementation cannot prove non-overlap for a layout it will reject it rather
than silently overwrite data. Empty-array behavior and rank-zero scalar
representation must be specified and tested. Caller-owned buffers must
outlive each operation invocation; reusable workspaces belong to operation
instances.

Type conversion will be explicit. Integer-to-floating conversion must reject
values that are not exactly representable where exactness is required, and
floating-to-integer conversion must check finiteness, fractional values and
range before casting. Conversions between complex and real values require an
explicit policy for the imaginary component; none is silently discarded.

Acceptance includes analytical tests for each type, complex layout tests,
rank/shape/stride and overflow failures, overlapping-output rejection,
existing Float64 regressions and measured performance comparisons. The
generic model must not claim NumPy compatibility or expose arbitrary
multidimensional layouts before those cases are verified.
