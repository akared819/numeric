# Local Linux validation — 2026-11-02

All 63 command checks corresponding to the repository's Linux CI test suite
passed on the Linux PC described below, including numerical correctness, IOC
integration, notification stress and network interruption/recovery checks.
The source under test was commit
`f8c23b1f5c32e152742683dacbd37cf5d62fb20d` (the remote default-branch HEAD at
validation time), without implementation changes.

## Environment

| Component | Value |
| --- | --- |
| OS / architecture | Ubuntu 26.04.1 LTS / `linux-x86_64` |
| Kernel | `7.0.0-38-generic` |
| CPU | Intel Core i7-11700, 8 cores / 16 threads |
| Compiler / language | GCC / G++ 15.2.0, C++14 |
| EPICS Base | Official 7.0.10 source archive, built locally |
| GSL | Official 2.8 source archive, shared libraries, built locally |
| Debugger | GDB 17.1 |

The existing `/home/plasma/epics/base` installation reports `7.0.10.1-DEV`.
It was left unchanged; this run targets the documented Base 7.0.10 release.
Dependencies and logs are in
`/home/plasma/Documents/numeric-linux-validation/`. No system packages were
installed. Base was built with `make -j8 COMMANDLINE_LIBRARY=EPICS`, because
readline development headers were absent. This differs from the Linux CI's
readline configuration; interactive readline behavior was not tested.

Downloaded archive SHA-256 values:

```text
6a99eeed15632c6354895b1dd542ed5a855c0f15d9ad1326c6fe2b2c9e423190  gsl-2.8.tar.gz
dca8a24a78fd5e48e659e510a4d40f04812aad9320a0a5317ab05099c27a6d22  base-7.0.10.tar.gz
```

The GSL checksum matched the value pinned in the repository's
[Linux CI workflow](../.github/workflows/ci.yml). The Base checksum records the
archive downloaded over HTTPS from the official release URL; it was not checked
against a separately published checksum.

## Executed checks

| Check | Result |
| --- | --- |
| GSL, EPICS Base and numeric builds | Passed |
| Compile-time rejection of GSL 2.7 headers | Passed |
| Runtime GSL linkage | Selected the local GSL 2.8 shared library; no missing libraries |
| Seven native numerical test programs | 7/7 passed |
| Ten IOC test programs, initialized and `NUMERIC_TEST_NO_INIT=1` | 20/20 passed |
| Separate-process example IOC and its two native clients | Passed |
| Core and isolated-IOC benchmark smoke checks (`64 100`) | Both passed |
| Invalid benchmark size (`0 100`) | Rejected with exit status 2 |
| Base-only control linkage | No numeric or GSL linkage |
| Base-only notification control under GDB | Passed, 500 callbacks and context teardowns |
| Local acknowledgment regression under GDB | 20/20 runs passed |
| Network ACK, restart and two controlled crash/recovery points | 5/5 runs passed |
| Connection interruption and retry | 3/3 runs passed |

The native phase completed 34 command checks, the GDB phase completed 21 and
the network phase completed eight: **63/63 passed**, with **1,765 passing TAP
assertions**, no failed TAP assertions and no skipped TAP tests. Native core
checks also use their own non-TAP assertions. A benchmark smoke run is not a
performance qualification.

## Local reproduction and evidence

`configure/RELEASE.local` selects the local Base 7.0.10 installation. From the
repository root, use the saved environment and runner:

```sh
source /home/plasma/Documents/numeric-linux-validation/env.sh
make -j8 GSL_DIR="$GSL_DIR"
python3 ../numeric-linux-validation/run-validation.py native
python3 ../numeric-linux-validation/run-validation.py stress
python3 ../numeric-linux-validation/run-validation.py network
```

The runner executes the existing CI commands sequentially, records exit status
and individual logs, rejects failed TAP assertions and bounds each command's
runtime. Communication tests use loopback ports 25064–25066. GDB needs permission
to debug its child processes. The runner is validation tooling; numeric itself
still has no Python runtime dependency.

Build logs are under `../numeric-linux-validation/logs/`; per-command logs and
`native-results.json`, `stress-results.json` and `network-results.json` are in
its `base-7.0.10/` subdirectory. `../numeric-linux-validation/manifest.json`
records the source revision, dependency checksums and build configuration.
These machine-local artifacts and `configure/RELEASE.local` are not committed.

## Observations and limits

- The numeric build completed with 48 warnings: 28 deprecated `rset`
  declarations in Base headers, 18 DBR macro redefinitions from including
  Base's database and Channel Access headers together, and two misleading
  indentation warnings in `numericTest.cpp:421` and `numericFitIocTest.cpp:22`.
  No build errors occurred. The source was not changed to suppress warnings.
- The known intermittent Base notification crash did not reproduce in this
  run. This does not establish that it is fixed; the scope in
  [Phase 2 acceptance](phase2-acceptance.md) still applies.
- This run covers the bundled test IOCs on this host. The existing Base
  development installation, facility IOCs, other architectures and remote-host
  networking were not validated.
