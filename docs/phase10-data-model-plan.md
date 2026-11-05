# Phase 10: typed array model — in progress

The existing Float64 `Input`, `Output` and operation APIs remain supported.
The separate `numericData.h` interface now provides typed views and exact
same-type element reads and writes independently of GSL. It is not yet wired
to numerical operations or EPICS records; cross-type conversion is not yet
implemented.

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
arrays must outlive each operation invocation.

Type conversion will be explicit. Integer-to-floating conversion must reject
values that are not exactly representable where exactness is required, and
floating-to-integer conversion must check finiteness, fractional values and
range before casting. Conversions between complex and real values require an
explicit policy for the imaginary component; none is silently discarded.

The native `numericDataTest` covers all six types, complex layout and unaligned
access, large Int64 values, empty views, invalid indices, capacity, overflow,
overlapping writes and preservation on failure. Acceptance also requires
cross-type conversion and numerical operation integration, additional
analytical tests, all platform CI, and performance comparisons. The model must
not claim NumPy compatibility or support arbitrary multidimensional layouts
before those cases are verified.
