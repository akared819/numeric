# numeric

Native C++ scientific computing foundation for EPICS, using external GNU Scientific Library (GSL). It includes a standalone core and aSub interfaces for batch and running statistics, including acknowledged single-producer streaming. No Python runtime is needed.

## Build and test

Target: Linux with EPICS Base 7.0.10, GSL 2.7.1 (Ubuntu 24.04), GNU C++ (C++11 or newer), GNU make and Perl. CI is configured to build those versions; other configurations are not yet claimed as supported. Build EPICS Base first ([EPICS installation instructions](https://docs.epics-controls.org/projects/base/en/b7.0.10/README.html)) and install GSL with headers and libraries. For Debian/Ubuntu, `sudo apt-get install build-essential libreadline-dev libgsl-dev` supplies build dependencies; the distribution GSL version may differ.

```sh
printf 'EPICS_BASE = /absolute/path/to/base-7.0.10\n' > configure/RELEASE.local
make GSL_DIR=/absolute/path/to/gsl
./bin/$(/absolute/path/to/base-7.0.10/startup/EpicsHostArch)/numericTest
```

Omit `GSL_DIR` when headers and libraries are in standard compiler paths. For a custom shared-library installation, configure the platform's runtime library search path as needed (e.g. `LD_LIBRARY_PATH` on Linux). `make clean` removes build products; installed headers and libraries appear under `include/` and `lib/<arch>/`.

Run the IOC integration tests from the repository root after building:

```sh
./bin/$(/absolute/path/to/base-7.0.10/startup/EpicsHostArch)/numericScalarIocTest
NUMERIC_TEST_NO_INIT=1 ./bin/$(/absolute/path/to/base-7.0.10/startup/EpicsHostArch)/numericScalarIocTest
./bin/$(/absolute/path/to/base-7.0.10/startup/EpicsHostArch)/numericArrayIocTest
NUMERIC_TEST_NO_INIT=1 ./bin/$(/absolute/path/to/base-7.0.10/startup/EpicsHostArch)/numericArrayIocTest
./bin/$(/absolute/path/to/base-7.0.10/startup/EpicsHostArch)/numericIocTest
NUMERIC_TEST_NO_INIT=1 ./bin/$(/absolute/path/to/base-7.0.10/startup/EpicsHostArch)/numericIocTest
./tests/example-ioc.sh $(/absolute/path/to/base-7.0.10/startup/EpicsHostArch)
./tests/network-ack.sh $(/absolute/path/to/base-7.0.10/startup/EpicsHostArch)
./tests/network-interruption.sh $(/absolute/path/to/base-7.0.10/startup/EpicsHostArch)
```

The `numericIocTest` application loads `numericApp/src/numericIocTest.db`; the dedicated record tests load their own example and test databases. They use the installed `dbd/numericIocTest.dbd`. The CA test scripts run clients against separate IOCs, including controlled IOC and connection failures. They use loopback ports 25064, 25065, and 25066, which must be available, and stop their test processes on exit. For setup and example databases see [aSub batch statistics](docs/asub-mean.md), [running statistics](docs/streaming.md), and the [acknowledged streaming protocol](docs/streaming-transport.md). The module also provides [radix-2 FFTs](docs/fft.md) and [moving mean, median and Gaussian smoothing](docs/window-statistics.md). The registry also provides the [error function](docs/special-functions.md), [vector norm](docs/linear-algebra.md), [standard normal CDF](docs/probability.md), [polynomial evaluation](docs/polynomials.md), [log1p](docs/elementary.md), [uniform histograms](docs/histograms.md), and [linear interpolation](docs/interpolation.md). The [numericScalar](docs/numeric-scalar.md) and [numericArray](docs/numeric-array.md) records provide dedicated scalar-result and array-result interfaces. See [Phase 4 acceptance and limits](docs/phase4-acceptance.md), [Phase 3 acceptance and limits](docs/phase3-acceptance.md), [architecture and IOC GSL setup](docs/architecture.md), [roadmap](docs/roadmap.md), [coverage](docs/gsl-coverage.md), and [licensing](docs/licensing.md).

## Example IOC

From the repository root after building, run the standalone example IOC:

```sh
./bin/$(/absolute/path/to/base-7.0.10/startup/EpicsHostArch)/numericExample examples/ioc/st.cmd
```

The script loads mean, running statistics, paired covariance, acknowledged streaming, FFT, window-statistics, numericScalar and numericArray records under the `demo:` prefix. It calls `numericInitialize` before `iocInit`; check the [process-wide GSL policy](docs/architecture.md#gsl-setup-for-an-ioc) when combining this module with other GSL users. In another terminal, with EPICS Base tools on `PATH`, try:

```sh
export PATH="$PWD/bin/$(/absolute/path/to/base-7.0.10/startup/EpicsHostArch):$PATH"
caput -a demo:input 3 1 3 5
caput demo:mean.PROC 1
caget demo:result
caput -a demo:scalarInput 4 1 2 3 4
caput demo:scalarMean.PROC 1
caget demo:scalarMean
caput demo:vectorNorm.PROC 1
caget demo:vectorNorm
caput demo:erfInput 1
caput demo:erf.PROC 1
caget demo:erf
caput demo:normalCdf.PROC 1
caget demo:normalCdf
caput demo:logOnePlus.PROC 1
caget demo:logOnePlus
caput -a demo:polyCoefficients 3 2 3 4
caput demo:polyPoint 2
caput demo:polyEval.PROC 1
caget demo:polyResult
caput -a demo:arrayInput 5 1 2 3 4 5
caput demo:arrayMean.PROC 1
caget -a demo:arrayMean
caput -a demo:arrayInput 5 0 0.25 0.5 0.75 0.999
caput demo:arrayHistogram.PROC 1
caget -a demo:arrayHistogram
caput -a demo:realFftInput 4 1 0 0 0
caput demo:realFftForward.PROC 1
caget -a demo:realFftOutput
caput -a demo:windowInput 5 1 1 99 1 1
caput demo:movingMedian.PROC 1
caget -a demo:movingMedianOutput
numericAckSend demo:event demo:acknowledgedStats 71 0 sample 2
numericAckSend demo:event demo:acknowledgedStats 71 1 sample 4
numericAckSend demo:event demo:acknowledgedStats 71 2 reset
```

Input links do not automatically process the records. For the streaming commands, the caller must follow the [single-producer acknowledgment protocol](docs/streaming-transport.md), including choosing a new session after an IOC restart. The example uses fixed `demo:` names; do not run another IOC with those names on the same CA network. Use `--headless` after the startup script to run without the interactive IOC shell. The startup script uses paths relative to the repository root.

The build also installs `numericAckSend`, a native Channel Access sender for the acknowledged streaming example. Its usage, retry behavior, and single-producer requirements are described in the [streaming transport guide](docs/streaming-transport.md#native-channel-access-sender).

## License

The numeric project is licensed under GPL-3.0-or-later. See [LICENSE](LICENSE) and [licensing considerations](docs/licensing.md). GSL and EPICS Base are separate external dependencies with their own licenses.
