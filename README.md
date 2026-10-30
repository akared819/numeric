# numeric

Native C++ scientific computing foundation for EPICS, using external GNU Scientific Library (GSL). The current increment provides a standalone core and one operation; it does not yet expose PVs or include an IOC record interface. No Python runtime is needed.

## Build and test

Target: Linux with EPICS Base 7.0.10, GSL 2.7.1 (Ubuntu 24.04), GNU C++ (C++11 or newer), GNU make and Perl. CI is configured to build those versions; other configurations are not yet claimed as supported. Build EPICS Base first ([EPICS installation instructions](https://docs.epics-controls.org/projects/base/en/b7.0.10/README.html)) and install GSL with headers and libraries. For Debian/Ubuntu, `sudo apt-get install build-essential libreadline-dev libgsl-dev` supplies build dependencies; the distribution GSL version may differ.

```sh
printf 'EPICS_BASE = /absolute/path/to/base-7.0.10\n' > configure/RELEASE.local
make GSL_DIR=/absolute/path/to/gsl
./bin/$(/absolute/path/to/base-7.0.10/startup/EpicsHostArch)/numericTest
```

Omit `GSL_DIR` when headers and libraries are in standard compiler paths. For a custom shared-library installation, configure the platform's runtime library search path as needed (e.g. `LD_LIBRARY_PATH` on Linux). `make clean` removes build products; installed headers and libraries appear under `include/` and `lib/<arch>/`.

The test executable is a minimal native test application; there is no IOC application or aSub support yet. See [architecture](docs/architecture.md), [roadmap](docs/roadmap.md), [coverage](docs/gsl-coverage.md), and [licensing](docs/licensing.md).
