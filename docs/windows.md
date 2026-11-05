# Windows native builds

The Windows target is **64-bit MinGW-w64**, `windows-x64-mingw`, with EPICS
Base **7.0.10**, GSL **2.8**, and C++14. MSYS2 supplies build tools and the
shell used by test scripts; the IOC, libraries and numerical programs are native
Windows PE executables and DLLs. They do not run under WSL and require no Python
interpreter. Visual Studio builds, 32-bit Windows and other MSYS2 toolchains are
not validated by this target. Do not mix MSVC EPICS libraries with MinGW objects.

## Build from an MSYS2 MinGW64 shell

Follow the [EPICS Windows installation guide](https://docs.epics-controls.org/en/latest/getting-started/installation-windows-msys2.html)
to install MSYS2. In an up-to-date **MinGW64** shell, install the build-time tools
and the [MinGW GSL package](https://packages.msys2.org/packages/mingw-w64-x86_64-gsl):

```sh
pacman -S --needed make perl tar curl mingw-w64-x86_64-gcc mingw-w64-x86_64-gsl
```

Run these commands from a numeric checkout in a path without spaces. Use forward
slashes and Windows drive paths in EPICS configuration files:

```sh
export EPICS_HOST_ARCH=windows-x64-mingw
mkdir -p .build-windows
cd .build-windows
curl -fsSLO https://epics-controls.org/download/base/base-7.0.10.tar.gz
tar -xzf base-7.0.10.tar.gz
make -C base-7.0.10 -j2 SUBMODULES=
cd ..
export EPICS_BASE="$(cygpath -m "$PWD/.build-windows/base-7.0.10")"
export GSL_DIR="$(cygpath -m /mingw64)"
sh /mingw64/bin/gsl-config --version # expected: 2.8
printf 'EPICS_BASE = %s\n' "$EPICS_BASE" > configure/RELEASE.local
make -j2 GSL_DIR="$GSL_DIR"
sh tests/windows.sh
```

`SUBMODULES=` omits optional PVA modules; numeric uses Base's Channel Access and
database libraries. A full Base build also works. An existing MinGW Base 7.0.10
installation can be used instead. `GSL_DIR` can
point to another matching MinGW GSL installation. The module uses EPICS library
directory variables rather than GNU-specific linker switches. The public core
API has DLL import/export annotations without depending on EPICS headers.
Standalone static core builds can include the headers without setting DLL macros.

## Run the example in PowerShell

For the paths above and MSYS2 installed at `C:\msys64`, run from the numeric root:

```powershell
$env:PATH = "$PWD\bin\windows-x64-mingw;$PWD\.build-windows\base-7.0.10\bin\windows-x64-mingw;C:\msys64\mingw64\bin;$env:PATH"
.\bin\windows-x64-mingw\numericExample.exe examples/ioc/st.cmd
```

Add a custom GSL installation's `bin` directory to `PATH` when applicable. In a
second terminal with the same DLL search path, use `caput.exe` and `caget.exe` as
shown in the README. Input updates still require explicit record processing.

## Verification and limits

`tests/windows.sh` runs the nine native numerical test programs, ten IOC test
programs both with and without GSL initialization, the minimum-GSL-version
rejection check, benchmark smoke/invalid-input checks, the example IOC, and the
multiple-input link reconnection tests and the acknowledged-stream restart,
crash and network-interruption tests. Each native and IOC test program has a
120-second timeout so a stalled calculation fails the runner. Run from the
repository root with `EPICS_BASE` and `GSL_DIR` set. It adds numeric, Base and GSL
DLL directories to the shell's `PATH`. The network tests use loopback ports
25064–25066 and 25140–25142; do not run these scripts concurrently.

The relay uses EPICS socket initialization, pointer-sized socket handles and
`select`, including Winsock cleanup. Test marker paths are converted to Windows
paths before passing them to native processes. Shell scripts and EPICS build
inputs have LF checkout rules in `.gitattributes`.

The [Windows workflow](../.github/workflows/windows.yml) builds and runs this same
suite on `windows-2022`. Adding the workflow is not evidence of a successful CI
run. Linux/macOS and the existing EPICS notification-crash limitation still
require their own validation; see [Phase 2 acceptance](phase2-acceptance.md).

### Local tool troubleshooting

Use one make implementation for the complete recursive build. An old local
MSYS2 installation can contain both an extensionless native `make` and MSYS
`make.exe`; mixing their Windows semaphore and POSIX jobservers fails during
parallel builds. Use a clean, current MSYS2 installation, or give one make binary
a unique name and pass its full path as `MAKE` throughout the build. Keep these
local workarounds outside the source files. A modern CI environment does not
depend on this PC's tool paths.

## Local validation evidence (2026-11-02)

Host: Windows 11 Home x64, version 10.0.26200, Intel Core i7-8565U. Compiler:
MSYS2 MinGW-w64 GCC 9.1.0 (Rev3). GSL: MSYS2 `mingw-w64-x86_64-gsl` 2.8-1,
extracted into the ignored `.build-windows/mingw64` prefix without changing the
system installation. Its official package SHA-256 is
`f8f854ac564b373032924b697795dd3f646d0ea5514a77a53cfb1bef8809e315` and was checked.

- Passed: independent core Windows DLL build (`g++ -std=c++14 -O2 -shared`),
  seven native numerical test programs linked against that DLL, and relay
  compilation with the Windows EPICS socket headers (`-Wall -Wextra`).
- Confirmed with `objdump -p`: the core DLL is `pei-x86-64` and imports Windows,
  MinGW and GSL libraries, without `msys-2.0.dll`.
- Passed: the standard EPICS Makefile build of numeric's two DLLs and all host
  executables against Base 7.0.10 from the official source archive, followed by
  `sh tests/windows.sh` (exit status 0). This includes seven native numerical
  programs, ten IOC programs each initialized and uninitialized, benchmark smoke
  and invalid-input checks, the example IOC, and all acknowledged-stream restart,
  in-flight/post-update crash and relay-interruption scenarios. The 35 completed
  TAP groups contain 1,006 passing assertions and no failures.
- The complete module run also passed the GSL 2.7 rejection test. No GDB
  notification-stress checks were run on Windows. The previously documented
  EPICS notification issue remains unresolved.
- At commit `68658c7`, [Linux CI](https://github.com/akared819/numeric/actions/runs/41384945992)
  and [macOS CI](https://github.com/akared819/numeric/actions/runs/41384945991)
  passed. Windows CI is still pending. Subsequent changes require their own CI
  results; these links are evidence for that specific commit.

Local commands (after extracting the dependencies and giving the MSYS make
binary a unique `.build-windows/msys-make.exe` name on this PC):

```sh
export EPICS_HOST_ARCH=windows-x64-mingw
export EPICS_BASE="$(cygpath -m "$PWD/.build-windows/base-7.0.10")"
export GSL_DIR="$(cygpath -m "$PWD/.build-windows/mingw64")"
printf 'EPICS_BASE = %s\n' "$EPICS_BASE" > configure/RELEASE.local
tool="$PWD/.build-windows/msys-make.exe"
"$tool" -j4 MAKE="$tool" GSL_DIR="$GSL_DIR"
sh tests/windows.sh
```

Acceptance: native build, numerical/invalid-input/lifetime tests, IOC integration,
public-interface documentation and coverage inventory are verified locally.
Candidate-specific CI remains a separate acceptance item until its runs finish.
The next portability increment would be independent validation of MSVC builds
with matching GSL and Base binaries; it is outside this MinGW increment.

## Develop validation (2026-11-05)

Updated this PC to `develop` commit `98b97b0` and rebuilt against the same
EPICS Base 7.0.10, GSL 2.8 and MinGW-w64 GCC 9.1.0 installations above.
The initial numerical run stalled in Welch PSD. Debugger inspection and a
standalone `std::hypot(double, double)` probe showed a self-recursive overload
in this PC's older C++ math headers, optimized into an infinite jump. This is
an observation about this local toolchain, not a claim that every GCC 9.1
installation has the defect. The raw power-spectrum path used the same call.

Both magnitude calculations now use the public GSL `gsl_hypot` API, keeping the
existing normalization, overflow checks and output-preservation behavior. No
public operation signature changed. Added analytical Welch regression cases
with amplitudes `1e200` and `1e-200` and compensating sample intervals to verify
that intermediate magnitude calculation does not overflow or underflow before
scaling. The Windows runner bounds each native and IOC test program to 120
seconds, so stalled computations report a failed run rather than waiting
indefinitely.

After these fixes, the standard EPICS build and `sh tests/windows.sh` completed
with exit status 0:

- All nine native numerical/data/typed-mean test programs passed, including
  the additional FFT magnitude cases and the new native callback APIs.
- All ten IOC test programs passed both initialized and uninitialized runs.
- The example IOC, multiple-input producer disconnect/reconnect tests, and
  acknowledged-stream restart, crash and relay-interruption tests passed.
- The 38 completed TAP groups contain 1,451 passing assertions and no failures.
  Native non-TAP checks, GSL 2.7 rejection and benchmark smoke/invalid-input
  checks also passed.
- `git diff --check` passed. Logs are retained locally under the ignored
  `.build-windows/validation-2026-11-05/` directory; the build and runner
  commands are the same as above.

The implementation and regression tests are committed together on `develop`;
candidate-specific CI results are available in that commit's GitHub checks.
The macOS allocation interposer and Linux GDB notification stress suite were
not run on this Windows PC. The existing EPICS notification limitation and
the separate MSVC validation requirement still apply.
