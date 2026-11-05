---
layout: default
title: Operation reference
permalink: /reference/
section: reference
alternate: /ja/reference/
---

<p class="eyebrow">numeric 0.2.0 reference</p>

# Operation reference

Each entry names the EPICS interface, ordered inputs, result and operation-specific conditions. Select an operation name to jump to it. The [full API and native interface index](#other-interfaces) below distinguishes features that do not use record fields. See the [NumPy reference](https://numpy.org/doc/stable/reference/generated/numpy.arange.html) for the documentation style that inspired this guide; numeric does not require Python.

## Reading record fields

| Field | Purpose |
| --- | --- |
| `OP` | Operation name; configured at database load. |
| `INP` | Input PV link for a single-input operation. |
| `INPA`–`INPP` | Ordered input PV links. A, B, C and D below refer to these links. |
| `ARGA`–`ARGP` | Ordered **configuration arguments** as strings, distinct from input PV links. An operation without arguments requires all ARG fields to remain blank. |
| `NMAX`, `OMAX` | Maximum input and output element capacities. `OMAX` applies to `numericArray`. |
| `VAL`, `NORD`, `NIN` | Result and element counts. `numericScalar.NORD` is its first input count; `numericArray.NORD` is its output count and `NIN` its first input count. |
| `PROC`, `STAT`, `SEVR` | Request processing and inspect alarm status and severity. |

For one input use `INP` **or** `INPA`, never both. Multiple links must be contiguous from A; they are read separately and do not form an atomic snapshot. Inputs must be PV links, not constant links. Updating an input or argument does not by itself process the record. Arguments are strict strings up to 38 characters; unused arguments cause an INVALID alarm. A nonempty ARG overrides its legacy field; a blank field restores the legacy value or documented default. [Record details](https://github.com/akared819/numeric/blob/main/docs/numeric-array.md).

A failed calculation can retain an old `VAL`; check `SEVR` before using it. Waveform capacity is distinct from valid element count. Inputs accept DOUBLE, FLOAT, LONG and exact local INT64; see [input conversion](https://github.com/akared819/numeric/blob/main/docs/numeric-scalar.md).

### Example: correlation

```db
record(numericScalar, "SIGNAL:CORR") {
    field(OP,   "correlation")
    field(INPA, "SIGNAL:X NPP NMS")
    field(INPB, "SIGNAL:Y NPP NMS")
    field(NMAX, "4096")
}
```

Update both waveform PVs, then process `SIGNAL:CORR.PROC` and inspect `VAL` and `SEVR`. Replace the example PV names with your actual PVs. For a running IOC example, see [multi-input.db](https://github.com/akared819/numeric/blob/main/examples/multi-input.db).

## Operations

{% for op in site.data.operations %}[`{{ op.name }}`](#{{ op.name }}){% unless forloop.last %} · {% endunless %}{% endfor %}

{% for op in site.data.operations %}
### `{{ op.name }}`

**Interface:** {% if op.interface == 'scalar' %}`numericScalar`{% elsif op.interface == 'array' %}`numericArray`{% else %}aSub / native registry; not a dedicated record{% endif %}.

**Inputs:** {{ op.inputs_en }}.

**Result and conditions:** {{ op.result_en }}

{% if op.args_en %}**Configuration arguments:** {{ op.args_en }}{% else %}**Configuration arguments:** None.{% endif %}

**Details and examples:** [{{ op.doc }}](https://github.com/akared819/numeric/blob/main/docs/{{ op.doc }}).
{% endfor %}

## Other interfaces

- **aSub:** [batch operations and multiple outputs](https://github.com/akared819/numeric/blob/main/docs/asub-mean.md), [FFT](https://github.com/akared819/numeric/blob/main/docs/fft.md), [fitting](https://github.com/akared819/numeric/blob/main/docs/fitting.md), and [acknowledged streaming](https://github.com/akared819/numeric/blob/main/docs/streaming-transport.md). The aSub fields and initialization differ from dedicated records; do not use the record input mapping above for aSub.
- **Native callbacks:** [`differentiateCentral`, `integrateAdaptive`, `findRootBrent`, `minimizeBrent`, `solveOdeRkf45`](https://github.com/akared819/numeric/blob/main/docs/numerical-differentiation.md) and [`fitNonlinear`](https://github.com/akared819/numeric/blob/main/docs/nonlinear-fitting.md). These are C++ functions, not record operations; callbacks and context must remain valid during the synchronous call.
- **Random sampling:** [`UniformRandomStream`](https://github.com/akared819/numeric/blob/main/docs/random-sampling.md) is a native, per-instance seeded stream; it is not a record operation.
- **Native registry and array views:** [architecture and ownership](https://github.com/akared819/numeric/blob/main/docs/architecture.md), [typed data model](https://github.com/akared819/numeric/blob/main/docs/phase10-data-model-plan.md), and the public headers [`numeric.h`](https://github.com/akared819/numeric/blob/main/numericApp/src/numeric.h), [`numericData.h`](https://github.com/akared819/numeric/blob/main/numericApp/src/numericData.h), [`numericTypedMean.h`](https://github.com/akared819/numeric/blob/main/numericApp/src/numericTypedMean.h), and [`numericAck.h`](https://github.com/akared819/numeric/blob/main/numericApp/src/numericAck.h).

GSL coverage is a subset, not the complete library; see the [coverage inventory](https://github.com/akared819/numeric/blob/main/docs/gsl-coverage.md).

## Native C++ API quick reference

These interfaces are synchronous and use caller-owned inputs and outputs unless stated otherwise. See the linked headers for exact signatures. `Result` contains a `Status` and a message; check it before consuming an output. Initialize the process-wide GSL policy at single-threaded startup with `initializeGsl()`; `isGslInitialized()` reports whether numeric performed initialization. The IOC command is `numericInitialize`.

| API | Inputs and result |
| --- | --- |
| `Registry::add`, `addWithoutArguments` | Register metadata and an operation factory; return whether registration succeeded. |
| `Registry::find`, `create`, `registerBuiltins` | Find metadata by name, create an independently owned operation, or register built-ins. Check a lookup or creation result for null. |
| `Operation::execute`, `reset` | Execute with input/output arrays and explicit counts, or reset instance state and resources. Buffers are owned by the caller. |
| `validateArgumentSchema`, `validateArguments` | Check a descriptor list or a complete ordered list of typed values, optionally with a cross-argument validator. |
| `resolveArguments`, `parseArgument`, `setLegacyArguments` | Resolve supplied values and trailing defaults into caller-owned storage; parse one strict ASCII value; or map resolved arguments into legacy scalar input slots. See [`numeric.h`](https://github.com/akared819/numeric/blob/main/numericApp/src/numeric.h) for capacities and failure rules. |
| `validateArray`, `validateWritableArray` | Validate typed array view shape, strides and byte capacity; rank 0 is scalar and maximum rank is 8. |
| `readElement`, `writeElement`, `convertElement` | Read/write a selected element or perform exact conversion. Writes require the same type; complex-to-real requires a zero imaginary component. |
| `TypedMean`, `TypedComplexMean` | Construct with a maximum element count; `reserve` prepares per-instance workspace, `execute` consumes an `ArrayView`, and `reset` clears workspace. Synchronize shared instances. |
| `AcknowledgedStream::create`, `submit` | Create a stream and submit a sample/reset event with session and sequence; the result includes an acknowledgment and statistics. Serialize access and follow the [single-producer protocol](https://github.com/akared819/numeric/blob/main/docs/streaming-transport.md). |
| `UniformRandomStream::initialize`, `fill`, `reset` | Initialize with a 32-bit seed, fill a caller-owned buffer, or restart at that seed. Instances are independent and not thread-safe. |
| `differentiateCentral` | Callback and context, point and positive step; returns derivative and estimated absolute error. |
| `integrateAdaptive` | Callback and context, finite increasing interval, tolerances and subdivision limit; returns integral and estimated absolute error. |
| `findRootBrent` | Callback and context, root bracket, tolerances and iteration limit; returns root and bracket error. |
| `minimizeBrent` | Callback and context, lower/initial/upper bracket, tolerances and iteration limit; returns minimum location and bracket error. |
| `solveOdeRkf45` | Derivative callback, time interval, initial state, dimension, step, tolerances and limit; writes the final state into caller-owned output. |
| `fitNonlinear` | Residual callback, initial parameters, residual count, tolerance and iteration limit; writes fitted parameters and sum of squared residuals. |

Output preservation, callback restrictions, numerical limits and examples are specified in the linked [callback guide](https://github.com/akared819/numeric/blob/main/docs/numerical-differentiation.md), [fitting guide](https://github.com/akared819/numeric/blob/main/docs/nonlinear-fitting.md) and [sampling guide](https://github.com/akared819/numeric/blob/main/docs/random-sampling.md). GSL error estimates are not guaranteed bounds.
