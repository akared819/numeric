# numeric

**Scientific computing for EPICS IOCs, powered by the GNU Scientific Library.**

`numeric` is a native C++ EPICS support module. It connects GSL calculations to scalar and waveform process variables through a small numerical core, aSub routines, and dedicated `numericScalar` and `numericArray` records. It requires no Python runtime.

> **Release candidate:** `0.1.0-rc.1` is the first candidate. See the [candidate notes](docs/release-candidate.md) for validation and limitations, and [GitHub releases](https://github.com/akared819/numeric/releases) for publication status.

## What it provides

| Capability | Examples | Guide |
| --- | --- | --- |
| Batch and streaming statistics | Mean, variance, covariance, correlation, running statistics | [Batch](docs/asub-mean.md), [streaming](docs/streaming.md) |
| Waveform calculations | Radix-2 FFT, moving mean and median, Gaussian smoothing, histogram | [FFT](docs/fft.md), [windows](docs/window-statistics.md), [histograms](docs/histograms.md) |
| Numerical functions | Interpolation, fitting, polynomial evaluation and integration, root finding, `erf`, `log1p`, normal CDF, vector norm | [GSL coverage](docs/gsl-coverage.md) |
| EPICS interfaces | aSub functions, scalar and array records, acknowledged single-producer streaming | [Scalar record](docs/numeric-scalar.md), [array record](docs/numeric-array.md), [streaming protocol](docs/streaming-transport.md) |

Coverage is a tested subset of GSL, not its complete API. See the [coverage inventory](docs/gsl-coverage.md) for exact operations and limits.

## Requirements

- EPICS Base **7.0.10**; Linux x86-64 and macOS arm64 are the CI targets.
- GSL **2.8 or newer**, including headers and libraries. GSL 2.8 is the current stable release and the version selected by CI. Older versions are rejected at compile time; newer versions are not yet validated.
- A C++14-capable GCC or Clang, GNU make, Perl, and the tools needed to build EPICS Base.

GSL is an external dependency and is not bundled. [GNU's GSL page](https://www.gnu.org/software/gsl/) lists the current release and installation resources.

### Install GSL 2.8

On macOS, `brew install gsl` currently provides GSL 2.8; check `$(brew --prefix gsl)/bin/gsl-config --version` before building. On Linux, distribution packages may be older. To build GSL 2.8 from the official source archive into a user-writable prefix:

```sh
curl -fsSLO https://ftp.gnu.org/gnu/gsl/gsl-2.8.tar.gz
echo '6a99eeed15632c6354895b1dd542ed5a855c0f15d9ad1326c6fe2b2c9e423190  gsl-2.8.tar.gz' | sha256sum -c -
tar -xzf gsl-2.8.tar.gz
cd gsl-2.8
./configure --prefix="$HOME/opt/gsl-2.8"
make -j2
make install
cd ..
```

The checksum command above is for Linux. On macOS, replace `sha256sum -c -` with `shasum -a 256 -c -`. For signed-release verification, follow [GNU's instructions](https://www.gnu.org/software/gsl/#downloading).

## Build and test

First build [EPICS Base 7.0.10](https://docs.epics-controls.org/projects/base/en/b7.0.10/README.html), then configure numeric from the repository root:

```sh
printf 'EPICS_BASE = /absolute/path/to/base-7.0.10\n' > configure/RELEASE.local
make -j2 GSL_DIR="$HOME/opt/gsl-2.8"
./bin/$(/absolute/path/to/base-7.0.10/startup/EpicsHostArch)/numericTest
```

On macOS with Homebrew, use `make -j2 GSL_DIR="$(brew --prefix gsl)"`. Omit `GSL_DIR` only when the compiler's standard paths contain GSL 2.8 or newer. For a custom shared-library installation, configure the platform's runtime library search path—for example, `export LD_LIBRARY_PATH="$HOME/opt/gsl-2.8/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"` on Linux—before running programs. Ensure the runtime loader selects the intended GSL library as well as the matching headers.

To exercise the example IOC and the separate-process Channel Access tests, from the repository root:

```sh
./tests/example-ioc.sh $(/absolute/path/to/base-7.0.10/startup/EpicsHostArch)
./tests/network-ack.sh $(/absolute/path/to/base-7.0.10/startup/EpicsHostArch)
./tests/network-interruption.sh $(/absolute/path/to/base-7.0.10/startup/EpicsHostArch)
```

These tests use loopback ports 25064–25066, which must be available. The [Linux](.github/workflows/ci.yml) and [macOS](.github/workflows/macos.yml) workflows show the full native and IOC test suites. Linux additionally runs the GDB notification stress checks. Build products install under `include/`, `lib/<arch>/`, `dbd/`, and `bin/<arch>/`; `make clean` removes build products.

For a step-by-step tour of the example databases, see the [examples and usage guide](docs-site/index.md). To publish that guide with GitHub Pages, set the repository’s Pages source to **GitHub Actions**; the documentation workflow deploys changes from `main` when that source is enabled and otherwise skips deployment.

## Try the example IOC

Start the example from the repository root:

```sh
./bin/$(/absolute/path/to/base-7.0.10/startup/EpicsHostArch)/numericExample examples/ioc/st.cmd
```

In another terminal with the EPICS Base tools on `PATH`:

```sh
caput -a demo:input 3 1 3 5
caput demo:mean.PROC 1
caget demo:result                 # expected: 3

caput -a demo:scalarInput 4 1 2 3 4
caput demo:scalarMean.PROC 1
caget demo:scalarMean             # expected: 2.5
```

The example loads records under `demo:`. Input links do **not** automatically process a calculation record; arrange processing explicitly for your application. For acknowledged streaming, follow the [single-producer protocol](docs/streaming-transport.md), including a new session after an IOC restart. `numericAckSend` is a native sender for that protocol.

The startup script calls `numericInitialize` before `iocInit`. All GSL users in the same IOC must agree on a non-terminating error handler and initialize it before concurrent GSL use; see the [IOC setup guide](docs/architecture.md#gsl-setup-for-an-ioc). Use `--headless` after the startup script for a noninteractive IOC.

## Validation and limitations

The [benchmark guide](docs/benchmarks.md) documents native and isolated-IOC measurements, including what their timings exclude. These observations are not hard-real-time guarantees.

An intermittent EPICS Base notification crash remains unresolved and has been reproduced without numeric or GSL linked. Its evidence and accepted scope are described in [Phase 2 acceptance](docs/phase2-acceptance.md). Test each target IOC and review its GSL handler policy and other modules before deployment.

For project status and release work, see the [architecture](docs/architecture.md), [roadmap](docs/roadmap.md), [Phase 6 acceptance](docs/phase6-acceptance.md), and [release-readiness checklist](docs/release-readiness.md).

## License

numeric is licensed under **GPL-3.0-or-later**. See [LICENSE](LICENSE) and the [licensing considerations](docs/licensing.md). GSL and EPICS Base are separate external dependencies with their own licenses; distribution of a combined IOC requires review of the applicable obligations.
