---
layout: default
title: Examples and usage
---

<p class="eyebrow">A practical guide</p>

# From data to results

<p class="lead">Build the example IOC, send it data, and explore the GSL-backed calculations available through numeric. Each example below uses the databases in this repository.</p>

<div class="callout" markdown="1">
**Before you start:** numeric uses GSL 2.8 or newer and EPICS Base 7.0.10. Follow the [build instructions](https://github.com/akared819/numeric#build-and-test), then return here to try the IOC.
</div>

## Quick start

From the repository root, start the bundled example IOC. Replace the Base path with your installation:

```sh
./bin/$(/absolute/path/to/base-7.0.10/startup/EpicsHostArch)/numericExample examples/ioc/st.cmd
```

Keep that terminal open. In a second terminal, change to the numeric repository root and add both EPICS Base and numeric tools to `PATH` (replace the Base path):

```sh
NUMERIC_EPICS_BASE=/absolute/path/to/base-7.0.10
NUMERIC_ARCH=$("$NUMERIC_EPICS_BASE/startup/EpicsHostArch")
export PATH="$NUMERIC_EPICS_BASE/bin/$NUMERIC_ARCH:$PWD/bin/$NUMERIC_ARCH:$PATH"
```

The IOC creates PVs with the `demo:` prefix.

Send three samples, process the mean record, and read the result:

```sh
caput -a demo:input 3 1 3 5
caput demo:mean.PROC 1
caget demo:result                 # 3
```

**Why process the record?** The waveform input link reads the current values when the calculation record processes. Writing the waveform does not by itself schedule the mean calculation. This rule applies to the batch examples below; the acknowledged streaming example uses a different, explicitly configured trigger.

## Examples

### Scalar result from a waveform

The `numericScalar` record in [scalar.db](https://github.com/akared819/numeric/blob/main/examples/scalar.db) reads a DOUBLE waveform and returns one value:

```sh
caput -a demo:scalarInput 4 1 2 3 4
caput demo:scalarMean.PROC 1
caget demo:scalarMean             # 2.5
```

The same input also feeds `demo:scalarVariance` and `demo:vectorNorm`; process the record you want before reading it. See the [scalar record reference](https://github.com/akared819/numeric/blob/main/docs/numeric-scalar.md) for input limits and alarm behavior.

### Histogram

The `numericArray` histogram in [array.db](https://github.com/akared819/numeric/blob/main/examples/array.db) has four uniform bins over `[0, 1)`. The lower bound is included and the upper bound is excluded:

```sh
caput -a demo:arrayInput 5 0 0.25 0.5 0.75 0.999
caput demo:arrayHistogram.PROC 1
caget -a demo:arrayHistogram      # four counts: 1 1 1 2
```

Samples outside the configured range cause an error. On failure the previous array value remains available; inspect `demo:arrayHistogram.SEVR` as well as the value. See [histogram behavior](https://github.com/akared819/numeric/blob/main/docs/histograms.md).

### Real-input FFT

The example FFT accepts four real input values and returns four complex values as eight interleaved real/imaginary doubles:

```sh
caput -a demo:realFftInput 4 1 0 0 0
caput demo:realFftForward.PROC 1
caget -a demo:realFftOutput       # 1 0 1 0 1 0 1 0
```

Only supported radix-2 lengths are accepted. See the [FFT guide](https://github.com/akared819/numeric/blob/main/docs/fft.md) for layouts, scaling and inverse transforms.

### Polynomial and interpolation

Polynomial coefficients are in ascending power order. For `2 + 3x + 4x²` at `x = 2`:

```sh
caput -a demo:polyCoefficients 3 2 3 4
caput demo:polyPoint 2
caput demo:polyEval.PROC 1
caget demo:polyResult             # 24
```

For linear interpolation, the x and y arrays must be the same length, x must be strictly increasing, and the query must be inside the x range:

```sh
caput -a demo:interpX 3 0 1 3
caput -a demo:interpY 3 0 2 6
caput demo:interpQuery 2
caput demo:interpEval.PROC 1
caget demo:interpResult           # 4
```

Inputs are read separately. Coordinate updates so the IOC cannot process an old array together with a new one. See [polynomials](https://github.com/akared819/numeric/blob/main/docs/polynomials.md) and [interpolation](https://github.com/akared819/numeric/blob/main/docs/interpolation.md).

### Fitting, integration and roots

These examples are also loaded by the bundled IOC:

```sh
# Unweighted line fit: output is c0, c1, cov00, cov01, cov11, sumsq.
caput -a demo:fitX 4 0 1 2 3
caput -a demo:fitY 4 1 2 2 4
caput demo:fitLinear.PROC 1
caget -a demo:fitResult           # approximately 0.9 0.9 0.245 -0.105 0.07 0.7

# Integral of 2 + 3x from 0 to 2.
caput -a demo:integralCoefficients 2 2 3
caput demo:integralLower 0
caput demo:integralUpper 2
caput demo:integralEval.PROC 1
caget demo:integralResult         # 10

# A root of x² - 2 bracketed between 0 and 2.
caput -a demo:rootCoefficients 3 -2 0 1
caput demo:rootLower 0
caput demo:rootUpper 2
caput demo:rootEval.PROC 1
caget demo:rootResult             # approximately 1.41421
```

See the [fitting](https://github.com/akared819/numeric/blob/main/docs/fitting.md), [integration](https://github.com/akared819/numeric/blob/main/docs/integration.md), and [roots](https://github.com/akared819/numeric/blob/main/docs/roots.md) references for numerical and input constraints.

## Streaming

The bundled [acknowledged streaming example](https://github.com/akared819/numeric/blob/main/examples/acknowledged-stats.db) is a coordinated, **single-producer** protocol. With numeric's `bin/<arch>` on `PATH`, one producer can send two samples and a reset:

```sh
numericAckSend demo:event demo:acknowledgedStats 71 0 sample 2
numericAckSend demo:event demo:acknowledgedStats 71 1 sample 4
numericAckSend demo:event demo:acknowledgedStats 71 2 reset
```

Choose a unique session for each stream lifetime. Start at sequence zero and wait for each acknowledgment before sending the next event. The example waveform's local forward link processes the consumer; **do not separately process the waveform or consumer**. The protocol is not a durable queue. After an IOC restart, use a new session and reconcile state as required by your application. Read the complete [streaming transport guide](https://github.com/akared819/numeric/blob/main/docs/streaming-transport.md) before integrating it.

## Database files

The `P` macro sets the PV prefix. The bundled [startup script](https://github.com/akared819/numeric/blob/main/examples/ioc/st.cmd) loads these files with `P=demo:`:

| Database | Purpose |
| --- | --- |
| [mean.db](https://github.com/akared819/numeric/blob/main/examples/mean.db), [scalar.db](https://github.com/akared819/numeric/blob/main/examples/scalar.db) | Batch mean and scalar-result records |
| [array.db](https://github.com/akared819/numeric/blob/main/examples/array.db), [window.db](https://github.com/akared819/numeric/blob/main/examples/window.db) | Array records, moving statistics, smoothing and histogram |
| [fft.db](https://github.com/akared819/numeric/blob/main/examples/fft.db) | Complex and real-input FFTs |
| [polynomial.db](https://github.com/akared819/numeric/blob/main/examples/polynomial.db), [interpolation.db](https://github.com/akared819/numeric/blob/main/examples/interpolation.db) | Polynomial evaluation and linear interpolation |
| [fitting.db](https://github.com/akared819/numeric/blob/main/examples/fitting.db), [integration.db](https://github.com/akared819/numeric/blob/main/examples/integration.db), [roots.db](https://github.com/akared819/numeric/blob/main/examples/roots.db) | Line fitting, polynomial integration and bracketed roots |
| [covariance-pairs.db](https://github.com/akared819/numeric/blob/main/examples/covariance-pairs.db) | Covariance from interleaved pairs |
| [running-stats.db](https://github.com/akared819/numeric/blob/main/examples/running-stats.db), [acknowledged-stats.db](https://github.com/akared819/numeric/blob/main/examples/acknowledged-stats.db) | Running statistics and acknowledged streaming |

Other example databases—`variance.db`, `stddev.db`, `minmax.db`, `covariance.db`, `correlation.db`, and `running-mean.db`—are available in the [examples directory](https://github.com/akared819/numeric/tree/main/examples) but are not loaded by the bundled startup script. Add the desired `dbLoadRecords` line **before `iocInit`** in your own startup script, and choose a prefix that does not conflict with existing PVs:

```text
dbLoadRecords("examples/variance.db", "P=myioc:")
```

## Using your own IOC

1. Build and link numeric support into the IOC and include the DBD files required by your interfaces. See the [scalar](https://github.com/akared819/numeric/blob/main/docs/numeric-scalar.md) and [array](https://github.com/akared819/numeric/blob/main/docs/numeric-array.md) record guides.
2. Register the IOC's record/device driver, call `numericInitialize`, and load the databases before `iocInit`. All GSL users in the IOC must agree on the [process-wide non-terminating error-handler policy](https://github.com/akared819/numeric/blob/main/docs/architecture.md#gsl-setup-for-an-ioc).
3. Set the input and output types and capacities for the operation. In the batch aSub examples, input buffers usually reserve one extra element to detect a truncated source waveform. `NMAX` and `OMAX` on dedicated records have their own documented meanings.
4. Arrange record processing and coordinate related input updates. Check the operation's documentation for allowed lengths, numerical conventions and workspace ownership.

## Troubleshooting

- **PV is not found:** confirm the example IOC is running, the database is loaded and the prefix matches. The bundled script uses `demo:`.
- **Value did not change:** check how the calculation record is processed. Most batch input updates do not trigger processing automatically.
- **Record has an INVALID alarm:** inspect `.STAT` and `.SEVR`, then check type, count, output capacity and operation-specific parameters. A failed calculation may preserve the previous result; do not treat that value as fresh without checking its alarm.
- **IOC combines multiple GSL users:** check that every component uses the same non-terminating handler. Numeric cannot prevent another component from changing it.

An intermittent EPICS Base notification crash remains unresolved, including in a Base-only control. See [Phase 2 acceptance](https://github.com/akared819/numeric/blob/main/docs/phase2-acceptance.md). Validate your own IOC and deployment environment before use.
