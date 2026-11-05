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
Boolean values must be zero or one. `validateArgumentSchema` checks descriptor
types, unique names and enum choices, ordering, bounds and defaults. The registry
rejects invalid schemas. `validateArguments` checks supplied values
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

## Remaining work

- Define complete metadata for every built-in and distinguish an explicitly
  empty argument list from an operation whose schema has not been migrated.
- Specify default application, argument-to-input mapping and enum names, then
  connect these to a parameterized execution API.
- Remove operation-name-specific parameter dispatch from record support only
  after its legacy-field precedence and runtime update behavior are tested.
- Verify all native and IOC regressions and required Linux, macOS and Windows
  checks before treating Phase 7 as complete.

Local increment validation on 2026-11-05: `make -j4
GSL_DIR=/tmp/numeric-gsl-2.8-install`, `numericTest`, `numericWindowTest`,
`numericHistogramTest` and `numericWindowIocTest` passed on macOS arm64 using
GSL 2.8 and the supplied EPICS Base checkout, which reported R7.0.11. The
window IOC test passed 67 assertions. This is not the release's clean Base
7.0.10 cross-platform acceptance run.
