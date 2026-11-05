# Native performance baseline

`numericBenchmark` compares `gsl_stats_mean`, numeric's registered `mean` operation and the typed-view `TypedMean` adapter over the same contiguous DOUBLE array. Build using the [README](../README.md), then run, for example:

```sh
./bin/$(/absolute/path/to/base-7.0.10/startup/EpicsHostArch)/numericBenchmark 1024 10000 "CPU model, operating system, architecture"
```

Arguments are the array length (1–16777216), iteration count (1–10000000), and a nonempty hardware description supplied by the operator. The program reports the compiler, GSL version, hardware description, array size, type, iteration count and elapsed nanoseconds per call. Use an accurate hardware description and record the EPICS Base version, build configuration, operating-system version, load and run date with results. CI runs a short correctness smoke test without a speed threshold; a shared CI runner is not a controlled performance environment.

The deterministic array contains repeating integers from -50 to 50. The benchmark initializes GSL, allocates the input, creates the registry and operations, reserves the typed workspace, and performs ten numeric warmups before timing. It then times direct GSL, the existing numeric operation and typed views in that order with a monotonic clock; all loops update a volatile checksum. Numeric checks buffer descriptors and samples, while direct GSL does not. The typed adapter validates and copies every element before calling GSL. The code paths do not allocate after the typed workspace is reserved; allocations are not instrumented. Timing excludes setup, EPICS record processing, IOC scheduling and network overhead. The checksum and a reference-result check detect a numerical mismatch. The fixed ordering, warm caches, machine activity and checksum overhead can bias results; repeat runs and vary sizes before drawing conclusions. These measurements do not establish hard-real-time behavior.

## Typed-view comparison

One run on 2026-11-05 on Apple M3, macOS 27.2 arm64, Apple LLVM 21.0.0, EPICS Base R7.0.11 and GSL 2.8 used the command above and no IOC or network overhead:

| DOUBLE array size | Iterations | Direct GSL ns/call | Existing core ns/call | Typed view ns/call |
| --- | ---: | ---: | ---: | ---: |
| 1,024 | 10,000 | 4,125.5 | 4,387.5 | 8,886.9 |
| 65,536 | 200 | 272,322.9 | 284,119.6 | 577,793.1 |

These single, uncontrolled measurements show the typed path taking roughly twice as long as the existing core path on this input. The typed path performs per-element type and finiteness checks and copies into its reusable workspace; the existing path reads the original DOUBLE buffer directly. This comparison does not establish which individual cost dominates. Earlier baseline measurements below used a different OS and Base version and are not a controlled regression comparison.

## Example measurement

One run on Apple M3, macOS 26.6.2 arm64, Apple LLVM 21.0.0 in C++14 mode, EPICS Base R7.0.10-151-g0cc912b17-dirty and GSL 2.8, with setup and allocations outside the timing and no IOC/network overhead:

| Algorithm and DOUBLE array size | Iterations | Direct GSL ns/call | numeric core ns/call |
| --- | ---: | ---: | ---: |
| mean, 1,024 | 10,000 | 3,877.1 | 4,279.4 |
| mean, 65,536 | 200 | 263,111.0 | 279,418.5 |

These are single observations from one machine, not a benchmark target or a comparison of equivalent validation. They show only the specific configuration and methodology above. A useful next step is repeatable sampling on isolated hardware and an IOC-level measurement.

## Isolated IOC record benchmark

`numericIocBenchmark` measures synchronous writes to the `PROC` field of a `numericScalar` mean record linked to a local DOUBLE waveform. It creates an [isolated test IOC](https://docs.epics-controls.org/projects/base/en/latest/iocInit_h.html) without a Channel Access server. Run it from the repository root after building:

```sh
./bin/$(/absolute/path/to/base-7.0.10/startup/EpicsHostArch)/numericIocBenchmark 1024 10000 "CPU model, operating system, architecture"
```

Arguments are array length (1–1048576, the record's current limit), iteration count (1–10000000) and a nonempty hardware description. Setup, record initialization, input allocation and the initial waveform write occur outside the timed section; ten warmup processes precede measurement. The timed loop includes the local database write to `PROC`, record processing, local input-link read and numeric validation/calculation. It excludes Channel Access and network overhead. The benchmark verifies the result against GSL and checks the alarm and valid element count after the loop. Allocations during record processing are **not instrumented**; the program does not claim there are none. A test IOC is not a production IOC workload, and no latency distribution, contention, monitors, network clients or hard-real-time guarantee is measured.

One run on Apple M3, macOS 26.6.2 arm64, Apple LLVM 21.0.0 in C++14 mode, the supplied EPICS Base development checkout and GSL 2.8:

| Algorithm and DOUBLE array size | Iterations | Isolated IOC ns/process |
| --- | ---: | ---: |
| numericScalar mean, 1,024 | 10,000 | 7,361.2 |
| numericScalar mean, 65,536 | 200 | 426,006.9 |

These single observations are not directly comparable to separate native runs as a precise overhead measurement: they ran at different times and do not control system load. Repeat on the intended target and report setup, allocations and EPICS/network boundaries with every result.

After adding typed-link input handling, one run on 2026-11-05 on Apple M3,
macOS 27.2 arm64, Apple LLVM 21.0.0, EPICS Base R7.0.11 and GSL 2.8 measured
7,438.6 ns/process at 1,024 elements (10,000 iterations) and 424,829.0
ns/process at 65,536 elements (200 iterations). This uses the same local
DOUBLE waveform benchmark and excludes network overhead; allocations were not
instrumented. The earlier table used a different OS and Base checkout, so this
is not a controlled before-and-after comparison. Additional FLOAT/LONG/INT64
conversion measurements remain necessary.

For a closer comparison on 2026-11-05, clean source from `main` commit
`1f71916` and the typed-link development implementation were built with the
same EPICS Base R7.0.11, GSL 2.8, Apple LLVM 21.0.0 and macOS 27.2 on an Apple
M3. Three runs alternated baseline and development for each size; the displayed
values are mean nanoseconds per processing call from each run:

| DOUBLE array size | Iterations per run | `main` ns/process | `develop` ns/process |
| --- | ---: | --- | --- |
| 1,024 | 10,000 | 7,362.2; 7,470.2; 7,589.9 | 7,552.9; 7,538.5; 7,470.2 |
| 65,536 | 200 | 422,829.4; 420,800.8; 419,297.7 | 426,510.0; 426,092.5; 419,140.8 |

These overlapping observations do not show a clear regression at these sizes,
but the machine was not isolated and the results are run averages, not
per-call latency distributions. This comparison covers local DOUBLE links only,
does not instrument allocations, and does not establish hard-real-time
behavior.
