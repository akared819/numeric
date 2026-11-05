# Phase 7: typed argument metadata (in progress)

The registry exposes optional `Metadata::arguments` descriptors for
`moving_mean`, `moving_median`, `gaussian_smooth`, and `histogram_uniform`. The original `inputs`
and `parameters` fields and execution interface remain unchanged. The
`argumentSchemaComplete` flag distinguishes a complete, possibly empty schema
from one that has not been migrated. Statistics, running statistics, radix-2
FFTs, the vector norm, `erf`, `log1p`, the normal CDF, and linear fitting are
explicitly registered without configuration arguments. Their numerical inputs
and constraints are unchanged. Polynomial evaluation, interpolation,
integration and roots still need review before their schemas are marked
complete.

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
`resolveArguments` instead fills a separate caller-owned array, applying
defaults to omitted optional trailing arguments, then runs the operation's
validator on the complete list. Missing defaults, short buffers and invalid
values fail. Its output contents are unspecified after failure.

Each descriptor can map to an index in the legacy scalar input list. Registered
mappings must identify distinct Float64 scalar inputs. This describes where an
adapter must place arguments; it does not itself convert integers to double or
guarantee exact representation. Adapters must reject unrepresentable integers.
`setLegacyArguments` provides such an adapter: it validates a complete argument
list, rejects integers that a double cannot represent exactly, and maps scalar
views into caller-owned `slots`. The views remain valid only while those slots
remain alive and unchanged. It does not allocate or apply defaults. The caller
must provide separate, non-overlapping value, slot and input buffers.

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

- Define complete metadata for every built-in.
- Specify enum names and connect argument metadata to every built-in, then
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
