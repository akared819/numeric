# aSub mean prototype

`numericMean` is a synchronous EPICS aSub routine for the existing `mean` operation. It reads DOUBLE array A with `NEA` valid elements in `NOA` allocated elements; it writes one DOUBLE to VALA and sets NEVA to 1. Configure FTA and FTVA as DOUBLE, NOA as the maximum number of input elements, and NOVA at least 1. Input may be a scalar (one element) or waveform. The input must be nonempty and finite. Calculation is repeated for the current input on each process, without persistent statistical state.

Set the IOC application `configure/RELEASE` NUMERIC path, include `numericSupport.dbd` in the application DBD, and link `numericIoc`, `numeric`, GSL and GSL CBLAS in addition to the EPICS Base IOC libraries. Call `numericInitialize` in the IOC startup script **after registering the IOC's record/device driver and before `iocInit` and any other concurrent GSL use**. This command initializes one shared, stateless mean instance and the process-global GSL error policy. It is idempotent; setup failures are logged and operations fail until initialization succeeds. As with the core, all GSL users in the process must agree not to change the error handler. Numeric cannot detect later changes by another component. The shared mean object is created once at startup and released at process shutdown; it contains no mutable computation state. Do not apply that sharing assumption to future operations.

Load [mean.db](../examples/mean.db) with a prefix. For example, a startup script after driver registration could contain:

```
numericInitialize
dbLoadRecords("path/to/numeric/examples/mean.db", "P=demo:")
iocInit
```

Writing samples to `demo:input` does not process `demo:mean`: explicitly process `demo:mean.PROC`, or configure a tested scan/forward-link arrangement appropriate to the IOC. The example uses an NPP input link so each processing fetches the current waveform. It deliberately does not claim to process each input update.

On successful calculation, VAL is 0 and output links are processed. Missing initialization, wrong types, invalid/empty data, insufficient capacity or a non-finite result set CALC/INVALID, clear NEVA and return -1; aSub does not write output links on nonzero return. The previous downstream output therefore remains available and consumers must inspect alarm/status. A failed input-link fetch is handled by EPICS record support before `numericMean` is called. See [EPICS aSub documentation](https://docs.epics-controls.org/projects/base/en/latest/aSubRecord.html). The C++ exceptions from startup or execution are caught before the EPICS C boundary.

The prototype covers only DOUBLE mean. It does not implement streaming, reset PVs, automatic event triggering, arbitrary operations, conversions, asynchronous processing, or record-owned workspace lifecycle. The example and IOC tests require native code, EPICS Base and GSL, not Python.
