# numericScalar record

`numericScalar` is a synchronous EPICS record that reads numeric scalar or waveform inputs and returns one DOUBLE scalar VAL using a compatible stateless numeric registry operation. Configure OP at database load time (default `mean`); supported names include `mean`, `variance`, `stddev`, `population_variance`, `population_stddev`, [`vector_norm`](linear-algebra.md), `correlation`, `covariance`, `polynomial_eval` and `pow_int`. OP must resolve to one or more Float64 scalar or array numerical inputs, optional mapped configuration arguments, and one Float64 scalar output; it must not be stateful. OP and NMAX are not modifiable at runtime. Scalar inputs require exactly one element. Multi-output operations are unsupported.

`interp_cspline` accepts ascending x coordinates in INPA, corresponding y
values in INPB and a scalar query in INPC. At least three finite points are
required and the query must lie inside the x range. It calls GSL's natural cubic
spline, whose endpoint second derivatives are zero. Each instance owns its
interpolation workspace; separate input links do not form an atomic snapshot.
See [GSL interpolation](https://www.gnu.org/software/gsl/doc/html/interp.html).

`weighted_mean` takes data in INPA and weights in INPB. Both arrays must have
equal, nonzero lengths and finite values; weights must be nonnegative with at
least one positive weight. Zero weights exclude the corresponding sample.
It uses GSL's weighted mean and rejects a weight sum outside the floating-point
range. Invalid input raises INVALID and retains the previous result. Inputs are
read sequentially, so the IOC must coordinate updates when a paired snapshot is
required. See [GSL weighted samples](https://www.gnu.org/software/gsl/doc/html/statistics.html#weighted-samples).

```db
record(numericScalar, "SIGNAL:WEIGHTED_MEAN") {
    field(OP,   "weighted_mean")
    field(INPA, "SIGNAL:DATA NPP NMS")
    field(INPB, "SIGNAL:WEIGHTS NPP NMS")
    field(NMAX, "4096")
}
```

`quantile_sorted` takes an ascending finite waveform in INPA and a finite
fraction between 0 and 1 in scalar INPB. GSL linearly interpolates between
adjacent samples; 0 and 1 select the minimum and maximum. Inputs are validated
without sorting or changing the waveform. At most INT_MAX samples are accepted
because of the GSL 2.8 quantile implementation's index conversion. For
example, data `[1, 2, 3, 4]` and fraction `0.75` give `3.25`.

For one numerical input, configure either the existing INP or the new INPA link. For multiple numerical inputs configure contiguous INPA, INPB and subsequent links, in registry input order excluding configuration arguments. A record that specifies INP and any INPA–INPP link, omits a required link, or specifies a surplus link fails initialization. Input links must be nonconstant. For example:

```db
record(numericScalar, "SIGNAL:CORR") {
    field(OP,   "correlation")
    field(INPA, "SIGNAL:X NPP NMS")
    field(INPB, "SIGNAL:Y NPP NMS")
    field(NMAX, "4096")
}
```

Both input waveforms must have the same valid length and meet the correlation operation's other constraints. Links are read in order during processing. Multiple CA inputs do not form an atomic snapshot; coordinate source updates and explicit record processing. Updating an input link does not automatically process the record.

Set NMAX to the largest waveform input count (1 through 1,048,576). For each waveform the record allocates NMAX+1 doubles during initialization and rejects an input longer than NMAX rather than silently truncating it; a scalar input has a two-element buffer to detect excess elements. Inputs accept DOUBLE, FLOAT, signed LONG and local database INT64 links. FLOAT and LONG convert exactly to DOUBLE; each INT64 value must be exactly representable as DOUBLE or processing fails. INT64 links are accepted only as local database links because classic Channel Access cannot guarantee exact Int64 transport. A remote source that presents itself as DOUBLE is treated as DOUBLE: upstream conversion or rounding is not detectable here. Other types are rejected. Temporary native-type buffers are allocated on first use and reused thereafter. NORD reports the number of elements used from the first input and becomes zero on failure. VAL and NORD are read-only to clients; PREC controls display precision.

`ARGA` through `ARGP` are writable string fields for ordered operation arguments. For [`pow_int`](elementary.md), ARGA selects an integer exponent; a blank field uses the default of 2. For operations without arguments, leave all ARG fields blank. A nonempty unused field or an invalid value raises CALC/INVALID on the next processing attempt and preserves VAL. Writing an ARG field does not itself process the record. The same 38-character limit, strict parsing, integer precision rules and processing snapshot semantics described for [numericArray](numeric-array.md) apply. Successfully parsed values are cached per record.

Call `numericInitialize` after registering support and before `iocInit`. On each process, the record fetches the input, validates and invokes the selected core operation. A successful calculation updates VAL and NORD, clears UDF and posts value/log events. The timestamp is refreshed on every processing attempt. A fetch, validation, numerical, setup or initialization failure preserves the last VAL, clears NORD and raises CALC/INVALID. Alarm changes are posted to monitors; a forward link is processed after each attempt, including failures, and must inspect alarm state. Input updates do not automatically schedule record processing. An invalid OP or NMAX causes record initialization to fail; processing that record subsequently raises an invalid alarm.

Each record owns a separate operation instance and one input buffer per link, allocated during record initialization. State is registered with `epicsAtExit` and is freed when the IOC calls `epicsExit` or `epicsExitCallAtExits`; no global computational workspace is shared. Returning from `main` or termination by a signal does not itself run these EPICS callbacks, although the OS reclaims process memory. The implementation does not support deleting and recreating databases dynamically in a running process, and it does not free its per-record buffers at IOC shutdown prior to process exit. Numerical execution does not allocate for the currently compatible batch statistics, `vector_norm`, `erf`, `normal_cdf` or `log1p`. The GSL process-wide policy and exception handling follow [architecture](architecture.md#gsl-setup-for-an-ioc).

For an IOC application, include both `numericSupport.dbd` and `numericScalarRecord.dbd` in its application DBD and link `numericIoc` and `numeric`. See [scalar.db](../examples/scalar.db) for mean and sample variance records. Write a waveform, explicitly process the desired record, and read VAL and SEVR. EPICS links can acquire data asynchronously and can report changing values; the record does not guarantee that every update is processed or that external data is stable across multiple PVs.

The additional link and argument fields change the generated record structure.
Rebuild the application DBD, generated record headers and linked record support
together when upgrading; do not mix binaries built against different versions.
Existing databases using only INP retain their documented behavior.

The standalone IOC also loads [multi-input.db](../examples/multi-input.db), with two-waveform correlation and waveform-plus-scalar polynomial examples. Set both inputs before triggering processing and inspect the result alarm.
