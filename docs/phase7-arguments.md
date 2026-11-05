# Phase 7: typed argument metadata (in progress)

The registry exposes optional `Metadata::arguments` descriptors for
`moving_mean`, `moving_median`, `gaussian_smooth`, and `histogram_uniform`. The original `inputs`
and `parameters` fields and execution interface remain unchanged. An empty
descriptor list does not yet establish that an operation has no parameters;
the remaining operations have not been migrated.

An argument descriptor names its position in the descriptor list, its type,
whether it is required, whether it has a default, inclusive bounds, and
enumeration choices. Integer, Boolean and enumeration values use `int64_t`,
while floating-point values use `double`. Values carry an explicit type tag;
Boolean values must be zero or one. `validateArguments` checks supplied values
and invokes an optional operation-specific validator after type/range checks.
Missing required arguments fail. Optional trailing arguments can be omitted;
the function does not apply defaults. The caller owns the argument buffers and
must supply a non-null pointer when the count is nonzero.

The window descriptors cover window and alpha bounds, and their operation
validator rejects even window sizes. The existing operation execution also
validates the window against the input length and all buffers. Metadata
validation alone is not a substitute for executing those checks.
The histogram descriptors check finite ordered bounds and the integer bin-count
range; the operation additionally checks output capacity and samples.

This is an API development increment, not completion of Phase 7. It does not
yet connect argument metadata to record fields or populate all built-ins.
Adding fields to the C++ `Metadata` structure changes its binary layout: native
applications must be rebuilt with the library; source compatibility for the
existing aggregate initializers is retained. No database interface is changed.
