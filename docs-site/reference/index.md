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
{% assign detail = site.data.operation_details[op.name] %}
### `{{ op.name }}`

**Interface:** {% if op.interface == 'scalar' %}`numericScalar`{% elsif op.interface == 'array' %}`numericArray`{% else %}aSub / native registry; not a dedicated record{% endif %}.

**Parameters:** {{ op.inputs_en }}.

{% for input in detail.types %}- `{{ input.name }}`: {% if input.shape == 'S' %}one finite numeric scalar{% else %}numeric waveform{% endif %}{% if op.interface == 'asub' %} (aSub input; use DOUBLE){% else %} (PV link, converted to DOUBLE; required, no default){% endif %}.
{% endfor %}

**Returns:** {% if detail.output_shape == 'S' %}one DOUBLE scalar in `VAL`{% elsif detail.output_shape == 'A' %}a DOUBLE array in `VAL`{% else %}{{ detail.output_shape.size }} DOUBLE scalar outputs, in the order described below{% endif %}. {{ op.result_en }}

{% if op.args_en %}**Configuration arguments:** {{ op.args_en }}{% else %}**Configuration arguments:** None.{% endif %}

**Errors:** Examples include {{ detail.error_en }}. Invalid buffers, non-finite inputs where unsupported, an uninitialized GSL policy, and numerical or allocation failures can also cause failure. Dedicated records raise `CALC/INVALID` and can retain the previous `VAL`; inspect `SEVR`. See the linked detailed specification for the complete rules.

**Example:** {% if op.interface == 'asub' %}For a fresh instance, using aSub inputs{% else %}With the following input PV values and argument settings{% endif %}:

{% for value in detail.example_inputs %}- `{{ value.name }}` = `{{ value.value }}`
{% endfor %}
Result: `{{ detail.example_output }}`{% if detail.output_shape.size > 1 %} (outputs in order){% endif %}. Values shown are approximate where floating-point rounding applies.{% if op.name == 'running_stats' %} `NaN` indicates the sample variance and standard deviation are undefined for the first sample.{% endif %}

{% if op.interface != 'asub' %}For example, configure this result record after creating the named source PVs (the sources and their values above are not created by this snippet):

```db
record(numeric{% if op.interface == 'scalar' %}Scalar{% else %}Array{% endif %}, "EXAMPLE:RESULT") {
    field(OP, "{{ op.name }}")
{% for input in detail.types %}    field({{ input.name }}, "EXAMPLE:{{ input.name }} NPP NMS")
{% endfor %}{% for value in detail.example_inputs %}{% if value.name contains 'ARG' %}    field({{ value.name }}, "{{ value.value }}")
{% endif %}{% endfor %}    field(NMAX, "128")
{% if op.interface == 'array' %}    field(OMAX, "128")
{% endif %}}
```

Set the source PVs to the example values, process `EXAMPLE:RESULT.PROC`, then read `VAL` and `SEVR`. Use the array form of `caget` for array results. Adjust the capacities for your own data. {% endif %}

**Further details:** [{{ op.doc }}](https://github.com/akared819/numeric/blob/main/docs/{{ op.doc }}).
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

## Native numerical API details

These are C++ APIs. Unless stated otherwise every parameter is required and has no default. Check `Result.status == Status::Ok` before using outputs. Keep callbacks and contexts alive during the call and initialize the GSL error policy first. Error estimates are not guaranteed bounds.

The [complete runnable example, including callback definitions](https://github.com/akared819/numeric/blob/main/examples/native-api-reference.cpp), uses `square(x)=x*x`, `rootFn(x)=x*x-2`, `minimumFn(x)=(x-2)^2`, `ode` for `dx/dt=x`, and `residual` for a three-point exponential model. The following calls use variables defined in that example.

{% for api in site.data.native_details %}
### `{{ api.name }}`

```cpp
{{ api.signature }}
```

**Parameters:** {{ api.parameters_en }}

**Returns:** `Result` (status and message). On success: {{ api.returns_en }}

**Errors:** {{ api.errors_en }}

**Example:**

```cpp
auto result = {{ api.example }};
// result.status == Status::Ok: {{ api.expected }}
```
{% endfor %}

## Native core and utility API details

These C++ APIs are not dedicated-record fields. The signature, parameters, return and failure conditions are listed below; parameters without an explicit C++ default are required. Each snippet is taken from the [complete executable example](https://github.com/akared819/numeric/blob/main/examples/native-core-reference.cpp). In real code, check every returned status and pointer and keep caller-owned buffers alive throughout each call.

### Shared types and ownership

| Type | Fields and meaning |
| --- | --- |
| `Input` | Caller-owned `const double* data`, valid element `count`, addressable `capacity`, element `stride`, `Type` and `Shape`. Capacity and valid count are distinct. |
| `Output` | Caller-owned `double* data`, `capacity`, operation-written `count`, `Type` and `Shape`. |
| `Descriptor` / `Type` / `Shape` | Input or output type and shape. Registry operations currently use `Float64` and `Scalar` or `Array`. |
| `Result` / `Status` | Status is `Ok`, `InvalidArgument`, `NumericalError` or `NotInitialized`; `message` is diagnostic text. |
| `ArgumentValue` / `ArgumentDescriptor` | Tagged integer, floating, boolean or enumeration value, plus name, required/default flags, limits, choices and mapping to legacy inputs. |
| `Metadata` | Name, category, input/output descriptors, argument schema, statefulness, ownership and error descriptions. A `Registry::find` result is registry-owned. |
| `ArrayView` / `MutableArrayView` | Caller-owned storage, capacity in bytes, `ElementType`, rank, extents and byte strides. Keep data, shape and stride arrays alive throughout use. |
| `ElementValue` | `ElementType` and integer or real/imaginary components. Complex components are real then imaginary. |
| `Event` / `Acknowledgment` | An event contains session, sequence, kind and value. An acknowledgment contains the event, seven statistics and a `replayed` flag. |

{% for api in site.data.native_core_details %}
### `{{ api.name }}`

```cpp
{{ api.signature }}
```

**Parameters and types:** {{ api.parameters_en }}

**Returns:** {{ api.returns_en }}

**Errors:** {{ api.errors_en }}

**Example:**

```cpp
{{ api.example }}
```
{% endfor %}


## aSub function details

Set the function name in an `aSub` record’s `SNAM`. A, B and C are aSub inputs fetched through links such as `INPA`; **they are not the dedicated-record `ARGA` fields**. Configure the used inputs and outputs as DOUBLE. For waveform inputs reserve one more `NO*` element than the largest source waveform and allocate sufficient `NOV*` output capacity to detect truncation. See each linked specification for exceptions and further constraints. Call `numericInitialize` before `iocInit`.

No operation-specific numerical input is filled automatically: configure each aSub input link or constant explicitly. Input updates generally do not schedule processing. An input-fetch failure can happen before the numeric callback, so inspect the record, links and data freshness. Previous output may remain on failure. Acknowledged streaming follows a separate processing protocol.

{% for api in site.data.asub_details %}
{% assign operation = site.data.operation_details[api.operation] %}
### `{{ api.name }}`

**Configuration:** `SNAM="{{ api.name }}"`. {{ api.inputs_en }}. Used inputs and outputs are DOUBLE.

**Parameter types and defaults:** A–C above are DOUBLE aSub inputs, not `ARGA`. Waveforms use `NO*` capacity and `NE*` valid count; scalars contain one element. No operation-specific numerical default is applied.

**Returns:** {{ api.outputs_en }}. Inspect the corresponding `NEV*` for valid output counts.

**Errors:** {{ operation.error_en }}, among others. Invalid types or capacities, numerical failures and missing GSL initialization can also fail. {% if api.name == 'numericRunningMean' or api.name == 'numericRunningStats' %}Reprocessing the same sequence is ignored with `VAL=1`.{% elsif api.name == 'numericAcknowledgedStats' %}The acknowledgment protocol also validates session and sequence.{% else %}Failures detected inside the function normally raise `CALC/INVALID` and do not update output links.{% endif %}

**Example:** {% for value in api.example_inputs %}`{{ value.name }}={{ value.value }}`{% unless forloop.last %}, {% endunless %}{% endfor %} → `{{ api.example_output }}`. Rounded values are approximate. {% if api.name == 'numericAcknowledgedStats' %}Submit this event with `numericAckSend demo:event demo:acknowledgedStats 1 0 sample 2`; do not issue a separate `.PROC` write.{% endif %}

[Example configuration: {{ api.db }}](https://github.com/akared819/numeric/blob/main/examples/{{ api.db }}){% if api.adapt_example %}; change its `SNAM` to `{{ api.name }}`{% endif %}. {% if api.name == 'numericRealFftForward' %}`FTVB=DOUBLE` and `NOVB >= 2N` are required; leave OUTB unconfigured. {% endif %}[Numerical operation details](#{{ api.operation }}).
{% endfor %}
