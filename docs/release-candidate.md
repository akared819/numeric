# numeric 0.1.0-rc.2 — release candidate

This document describes the second candidate. See [GitHub releases](https://github.com/akared819/numeric/releases) for publication status. The candidate commit and actual CI results are recorded in its pull request and must be rechecked if the commit changes.

## Changes since 0.1.0-rc.1

- Add native Windows x64 MinGW builds, DLL exports and library discovery, Windows socket support for the test relay, and portable test marker paths and line-ending handling. The [Windows guide](windows.md) records the local build and numerical, IOC and network validation results.
- Add an example database tour and usage guide, including troubleshooting and deployment advice.
- Record [local Linux validation](linux-validation.md) on Ubuntu 26.04.1 x86-64 with GCC 15.2.0, EPICS Base 7.0.10 and GSL 2.8: all 63 command checks passed, including 20 GDB acknowledgment regressions, five network ACK/restart/crash runs and three connection-interruption runs. The implementation under test was `f8c23b1f5c32e152742683dacbd37cf5d62fb20d`; the candidate update changes documentation only.

The known Base notification crash remains unresolved. The successful local runs do not replace candidate-specific CI or target-IOC validation.

## What is included

- Native numerical core with an operation registry, validation, error reporting and per-instance resource ownership.
- EPICS aSub integration, `numericScalar` and `numericArray` records, and an example IOC.
- Batch and running statistics, an acknowledged single-producer streaming protocol, radix-2 FFTs, moving statistics and selected GSL functions. The [coverage inventory](gsl-coverage.md) identifies the tested subset.
- Native and isolated-IOC [benchmarks](benchmarks.md), Linux, macOS and Windows MinGW CI workflows, and a [release-readiness checklist](release-readiness.md).

## Dependencies and compatibility

The candidate targets EPICS Base 7.0.10, a C++14-capable GCC or Clang, and GSL 2.8. Platforms with CI workflows are Linux x86-64, macOS arm64 and native Windows x64 MinGW; the [Windows guide](windows.md) documents its matching-toolchain requirement and exclusions. **GSL 2.8 or newer is required at compile time**; earlier development builds were tested with GSL 2.7.1 on Linux. Upgrade the headers and runtime libraries together and configure their paths as described in the [README](../README.md). GSL 2.8 is the CI target; later GSL versions and other EPICS releases are not yet validated for this candidate.

For IOC applications using dedicated record types, rebuild record support and DBDs together with the candidate. There is no previous numbered numeric release against which binary compatibility is promised. The native C++ API and EPICS interfaces are preliminary and may change in response to candidate feedback.

## Known limitations

- The intermittent EPICS Base notification crash remains unresolved and has been reproduced without numeric or GSL linked. Passing tests do not prove it fixed; see [Phase 2 acceptance](phase2-acceptance.md).
- Every IOC integrator must arrange and maintain a shared non-terminating GSL error-handler policy before concurrent use. Another module can replace that process-wide handler; see [architecture](architecture.md#gsl-setup-for-an-ioc).
- Streaming is a cooperative single-producer protocol, not a durable queue. Inputs fetched from separate links require producer coordination; input links do not guarantee a process event for each update.
- GSL coverage and platform validation are limited. No hard-real-time guarantee, network-latency guarantee, arbitrary callback support or complete GSL coverage is claimed.
- Release distribution requires review of GPL-3.0-or-later obligations and third-party notices for the exact artifacts; see [licensing](licensing.md).

## Release review

The candidate review records the exact commit, source archive checksum, dependency and compiler versions, local results and Linux/macOS CI links. Reviewers assess compatibility, known limitations and distribution obligations for the intended artifacts and target IOCs. See the complete [release-readiness checklist](release-readiness.md) for the checks to perform before publication.
