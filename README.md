# numeric

Native C++ scientific computing foundation for EPICS, using external GNU Scientific Library (GSL). The current increment provides a standalone core and one operation; it includes a prototype aSub interface for mean and a native running-mean operation. No Python runtime is needed.

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
./bin/$(/absolute/path/to/base-7.0.10/startup/EpicsHostArch)/numericIocTest
NUMERIC_TEST_NO_INIT=1 ./bin/$(/absolute/path/to/base-7.0.10/startup/EpicsHostArch)/numericIocTest
```

The test IOC loads `numericApp/src/numericIocTest.db` and the installed `dbd/numericIocTest.dbd`. For setup and an example database see [aSub mean](docs/asub-mean.md). The native streaming API is described in [running mean](docs/streaming.md). See [architecture](docs/architecture.md), [roadmap](docs/roadmap.md), [coverage](docs/gsl-coverage.md), and [licensing](docs/licensing.md).

## License

The numeric project is licensed under GPL-3.0-or-later. See [LICENSE](LICENSE) and [licensing considerations](docs/licensing.md). GSL and EPICS Base are separate external dependencies with their own licenses.
