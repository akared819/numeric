# numericScalar record

`numericScalar` is a synchronous EPICS record that reads DOUBLE scalar or waveform inputs and returns one scalar VAL using a compatible stateless numeric registry operation. Configure OP at database load time (default `mean`); supported names include `mean`, `variance`, `stddev`, `population_variance`, `population_stddev`, [`vector_norm`](linear-algebra.md), `correlation`, `covariance` and `polynomial_eval`. OP must resolve to one or more DOUBLE scalar or array inputs and one DOUBLE scalar output, must not be stateful, and must have no configuration arguments. OP and NMAX are not modifiable at runtime. Scalar inputs require exactly one element. Multi-output operations are unsupported.

For one input, configure either the existing INP or the new INPA link. For multiple inputs configure contiguous INPA, INPB and subsequent links, in registry input order. A record that specifies INP and any INPA–INPP link, omits a required link, or specifies a surplus link fails initialization. All input links must be nonconstant and report DOUBLE type. For example:

```db
record(numericScalar, "SIGNAL:CORR") {
    field(OP,   "correlation")
    field(INPA, "SIGNAL:X NPP NMS")
    field(INPB, "SIGNAL:Y NPP NMS")
    field(NMAX, "4096")
}
```

Both input waveforms must have the same valid length and meet the correlation operation's other constraints. Links are read in order during processing. Multiple CA inputs do not form an atomic snapshot; coordinate source updates and explicit record processing. Updating an input link does not automatically process the record.

Set NMAX to the largest waveform input count (1 through 1,048,576). For each waveform the record allocates NMAX+1 doubles during initialization and rejects an input longer than NMAX rather than silently truncating it; a scalar input has a two-element buffer to detect excess elements. Other numeric types are not converted. NORD reports the number of elements used from the first input and becomes zero on failure. VAL and NORD are read-only to clients; PREC controls display precision.

`ARGA` through `ARGP` are writable string fields reserved for operation arguments. Current scalar-compatible operations have no configuration arguments: leave all ARG fields blank. If any is nonempty, the next processing attempt raises CALC/INVALID and preserves VAL. Writing an ARG field does not itself process the record.

Call `numericInitialize` after registering support and before `iocInit`. On each process, the record fetches the input, validates and invokes the selected core operation. A successful calculation updates VAL and NORD, clears UDF and posts value/log events. The timestamp is refreshed on every processing attempt. A fetch, validation, numerical, setup or initialization failure preserves the last VAL, clears NORD and raises CALC/INVALID. Alarm changes are posted to monitors; a forward link is processed after each attempt, including failures, and must inspect alarm state. Input updates do not automatically schedule record processing. An invalid OP or NMAX causes record initialization to fail; processing that record subsequently raises an invalid alarm.

Each record owns a separate operation instance and one input buffer per link, allocated during record initialization. State is registered with `epicsAtExit` and is freed when the IOC calls `epicsExit` or `epicsExitCallAtExits`; no global computational workspace is shared. Returning from `main` or termination by a signal does not itself run these EPICS callbacks, although the OS reclaims process memory. The implementation does not support deleting and recreating databases dynamically in a running process, and it does not free its per-record buffers at IOC shutdown prior to process exit. Numerical execution does not allocate for the currently compatible batch statistics, `vector_norm`, `erf`, `normal_cdf` or `log1p`. The GSL process-wide policy and exception handling follow [architecture](architecture.md#gsl-setup-for-an-ioc).

For an IOC application, include both `numericSupport.dbd` and `numericScalarRecord.dbd` in its application DBD and link `numericIoc` and `numeric`. See [scalar.db](../examples/scalar.db) for mean and sample variance records. Write a waveform, explicitly process the desired record, and read VAL and SEVR. EPICS links can acquire data asynchronously and can report changing values; the record does not guarantee that every update is processed or that external data is stable across multiple PVs.

The standalone IOC also loads [multi-input.db](../examples/multi-input.db), with two-waveform correlation and waveform-plus-scalar polynomial examples. Set both inputs before triggering processing and inspect the result alarm.
