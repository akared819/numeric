# Roadmap and acceptance

## This increment: Phase 0 and minimal Phase 1

- [x] EPICS module layout, configurable Base and GSL, native test application, Linux CI definition.
- [x] Registry, metadata, descriptors, operation lifecycle, validation and GSL mean adapter.
- [x] Architecture, coverage inventory and licensing considerations.
- [ ] Verify the pinned Linux CI build and test in GitHub Actions.
- [ ] Select a project license after review of GSL linking and redistribution obligations.
- [ ] Validate against a clean EPICS Base 7.0.10 release on Linux; local results are environment-specific.

Local verification: `make GSL_DIR=/tmp/numeric-gsl-install` and `./bin/darwin-aarch64/numericTest` passed on macOS using GSL 2.8 and EPICS Base R7.0.10-151-g0cc912b17-dirty. Acceptance also requires green Linux CI against the clean selected Base release. The reproducible test command is in the README.

## Next work

Complete Phase 1 by reviewing the public data model, parameter schema, callback and error/lifetime contracts, and process-wide GSL policy with IOC integrators. Only after Phase 0 acceptance and unresolved decisions are approved, proceed to Phase 2: aSub integration, batch and streaming statistics, explicit triggering, reset, alarms and IOC tests. Phase 3 covers FFT and filtering; Phase 4 evaluates dedicated records; Phase 5 expands GSL coverage; Phase 6 focuses on measured performance and portability. None of those interfaces is implemented yet.
