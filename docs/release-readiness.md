# Release-readiness checklist

This is a checklist for preparing a future numeric release; it does not declare that a release has been approved or published. Complete it for the exact candidate commit and record evidence. A passing development PR or a benchmark observation alone is not a release approval.

## Candidate and compatibility

- [ ] Choose a version, identify the release owner and record the candidate commit and release date.
- [ ] Review public interfaces and record compatibility, including numericScalar/numericArray DBD and record support rebuild requirements. Document any breaking changes and migration steps.
- [ ] Record exact operating systems, CPU architectures, compiler versions and EPICS Base and GSL versions actually tested. Verify the target IOC configuration separately; do not extrapolate CI to untested platforms.
- [ ] Confirm the build and install instructions from a clean checkout and run the standalone example IOC. Record commands and results.
- [ ] Require all Linux and macOS CI checks to pass for the candidate commit, including native and initialized/uninitialized IOC tests. Record failed or rerun jobs and their causes.
- [ ] Review the known intermittent EPICS Base notification crash and decide whether its current documented limitation is acceptable for the release and intended deployments. Passing stress tests does not establish that it is fixed; see [Phase 2 acceptance](phase2-acceptance.md).

## Safety, coverage and performance

- [ ] Verify numeric initialization and the process-wide GSL error-handler agreement for each intended IOC integration; see [architecture](architecture.md#gsl-setup-for-an-ioc).
- [ ] Review input validation, alarm behavior, resource lifetime, concurrency and operation documentation for changes since the previous release.
- [ ] Check [GSL coverage](gsl-coverage.md) and the example databases for accuracy; list unsupported functionality without implying complete GSL coverage.
- [ ] Run applicable [benchmarks](benchmarks.md) on representative hardware. Record algorithm, data type, size, hardware, timing and allocation, IOC and network boundaries; do not infer a hard-real-time guarantee.

## Distribution and publication

- [ ] Review [licensing](licensing.md) for the exact distribution, including GPL-3.0-or-later obligations, corresponding source, and third-party license and notice requirements.
- [ ] Prepare release notes listing features, supported configurations, upgrade instructions, known defects and unresolved decisions.
- [ ] Determine source and binary artifacts, build provenance and checksums; verify the artifacts and their dependencies from a clean environment.
- [ ] Review the proposed tag, release notes and artifacts with the release owner before creating a tag or publishing a release.

## Evidence to preserve

Record the candidate commit, CI run links, exact dependency and compiler versions, build commands, test and benchmark results, platform limits, accepted issues, licensing review, artifact checksums and the release owner's decision. If a check fails or a required decision is missing, leave it unchecked and document who will resolve it. Re-run affected checks when the candidate changes.
