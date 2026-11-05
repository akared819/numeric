# Linux validation of the 0.2.0 development branch

Local verification on 2026-11-05 tested implementation commit
`b00becb2b914b06aca8372d3838c2b30f9d762f6`. This is evidence for that
commit and host, not a 0.2.0 release declaration. The previous 0.1.0-rc.2
validation is recorded separately in [Linux validation](linux-validation.md).

## Environment and build

- Ubuntu 26.04.1 LTS, Linux `7.0.0-38-generic`, x86-64, Intel Core i7-11700.
- GCC/G++ 15.2.0, EPICS Base 7.0.10 and GSL 2.8 shared libraries.
- Base was built with `COMMANDLINE_LIBRARY=EPICS`; interactive readline was
  not tested. Numeric was cleaned and rebuilt with GSL 2.8 using:

  ```sh
  make clean
  make -j8 GSL_DIR=/path/to/gsl-2.8
  ```

  `configure/RELEASE.local` selected the Base 7.0.10 installation. The
  GSL 2.7 header rejection and runtime GSL library selection checks passed.

The build succeeded with 50 warnings: 30 deprecated `rset` declarations in
Base headers, 18 DBR macro redefinitions from combined Base database/CA
headers, and two misleading-indentation warnings in test code. No compile
errors occurred.

## Completed tests

The [Linux CI workflow](../.github/workflows/ci.yml) describes the commands
and repetition counts used for the native, IOC, GDB and network tests. All
66 command checks passed: 37 native/build-configuration checks, 21 GDB checks
and eight network checks. Their output contained 2,210 passing TAP assertions,
no failed TAP assertions and no skipped TAP tests; the native core programs
also use their own non-TAP assertions.

- All nine native numerical test programs passed, including typed data,
  FFT, spectral analysis and callback tests.
- All ten IOC test programs passed both with normal initialization and with
  `NUMERIC_TEST_NO_INIT=1` (20 runs).
- The example IOC and the separate producer/consumer disconnect and
  reconnect test passed. Benchmark smoke checks passed and a zero-sized
  benchmark input was rejected.
- The Base-only binary did not link numeric or GSL. Its 500 notification
  callbacks and context teardowns passed under GDB; the local acknowledgment
  regression passed 20 runs under GDB.
- The separate-process ACK, restart and two controlled crash/recovery tests
  passed five runs. Connection interruption and retry passed three runs.
  The tests used loopback ports 25064–25066 and 25140–25142.

The per-command output and JSON results for this machine are stored under
`/home/plasma/Documents/numeric-linux-validation/logs/develop-b00becb/`;
they are not part of the repository.

## Scope and limitations

The known intermittent EPICS Base notification crash did not reproduce in
the completed GDB runs; this does not establish that it is fixed. These tests
cover this Linux host and the bundled test IOCs. Production IOCs, other Linux
architectures, macOS, Windows and remote-host networking were not tested in
this run. The existing Base `7.0.10.1-DEV` installation was not modified or
tested. See the [development notes](release-0.2.0-draft.md) and
[Phase 2 acceptance](phase2-acceptance.md) for remaining integration limits.
