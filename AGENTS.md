# AGENTS.md — numeric

## 1. Project overview

**numeric — Scientific Computing for EPICS**

Repository: https://github.com/akared819/numeric

`numeric` is an EPICS support module that exposes numerical and scientific computing capabilities from the GNU Scientific Library (GSL) to EPICS Input/Output Controllers (IOCs).

The long-term goal is to make as much of the public GSL functionality as practical accessible through consistent EPICS interfaces.

The project is implemented in C/C++ and must not require Python at runtime.

### Goals

- Support scalar and waveform process variables (PVs).
- Support batch and streaming numerical computations.
- Provide a consistent interface to GSL algorithms.
- Minimize processing latency and unnecessary allocations.
- Integrate naturally with EPICS Base and its build system.
- Support extensible function registration.
- Expose meaningful errors through EPICS mechanisms.
- Maintain an explicit inventory of GSL API coverage.
- Provide reproducible tests, examples, and benchmarks.

### Non-goals

- Reimplementing algorithms already provided by GSL without a justified reason.
- Introducing Python, NumPy, or SciPy runtime dependencies.
- Implementing every GSL function in the initial version.
- Guaranteeing hard real-time execution.
- Introducing mandatory dependencies beyond EPICS Base and GSL without approval.

## 2. Core architectural principles

Use three logical layers:

1. **numeric Core** — operation registry, argument validation, buffer and state management, error reporting.
2. **GSL bindings** — thin adapters around GSL functions.
3. **EPICS interfaces** — aSub support initially, followed by dedicated record types after review.

Keep EPICS-specific code separate from GSL wrappers wherever practical.

The numerical core must be independently testable without running an IOC.

Do not create a separate EPICS record type for every GSL function.

Do not expose raw GSL workspace pointers through public EPICS fields.

Prefer a small, stable API over a large number of unrelated interfaces.

## 3. Technology requirements

- Language: C++ for the core, with C-compatible interfaces where EPICS requires them.
- EPICS: EPICS Base 7.x as the initial target.
- Scientific backend: GNU Scientific Library (GSL).
- Build system: standard EPICS Makefile-based build system.
- Initial development platform: Linux.
- Runtime: compiled native code, without a Python interpreter.

Use supported public APIs from EPICS Base and GSL.

Do not depend on undocumented internal structures or vendored copies of GSL.

Document supported compiler, EPICS Base, and GSL versions. Confirm compatibility through CI.

## 4. Function registry

Implement a common operation registry.

Every registered operation must describe:

- Unique operation name
- Category
- Required input types and number of inputs
- Output types and number of outputs
- Parameter schema
- Whether it is stateful
- Initialization and cleanup requirements
- Execution callback
- Error reporting behavior

Design the registry to support:

- Scalar to scalar
- Array to scalar
- Array to array
- Multiple inputs and outputs
- Stateful streaming operations
- Solver operations using user-provided callbacks

Do not assume all GSL functions have identical signatures.

Use explicit adapters for functions with special memory, callback, or workspace requirements.

## 5. Input and output handling

Support EPICS scalar and waveform data as primary input sources.

Initially, prioritize DOUBLE data.

Provide explicit and safe conversion rules for other numeric EPICS types.

Validate array sizes, element counts, stride, types, and output capacity before calculation.

Distinguish allocated array capacity from the number of valid elements.

Do not silently truncate data.

Specify how complex data and multidimensional arrays are represented before introducing them.

Avoid unnecessary copies, but never sacrifice buffer ownership or thread safety for zero-copy behavior.

## 6. Batch and streaming operations

### Batch mode

Batch operations process a complete input array on each invocation.

Examples:

- mean
- variance
- standard deviation
- FFT
- filtering
- interpolation

Use GSL's existing batch APIs wherever applicable.

### Streaming mode

Streaming operations update persistent state as samples arrive.

Use `gsl_rstat` where applicable.

Examples:

- running mean
- running variance
- running standard deviation
- running RMS
- running minimum and maximum

Each stateful operation instance must own its state or use an explicitly synchronized ownership model.

Provide reset functionality.

Document first-sample behavior, sample counting, and sample-versus-population variance conventions.

Do not assume an EPICS input link automatically triggers processing on every PV update.

Prevent unintended double counting caused by record processing or link configuration.

## 7. Performance and concurrency

Performance is a core project requirement.

- Avoid heap allocations in repeatedly executed numerical paths when practical.
- Allocate reusable GSL workspaces during initialization.
- Reuse FFT workspaces and other persistent resources.
- Avoid global mutable state for individual operation instances.
- Keep processing predictable under repeated inputs.
- Use thread-safe lifetime management.
- Measure performance instead of assuming an implementation is faster.
- Keep numerical work separate from unnecessary IOC/network overhead.

Do not perform potentially expensive solver operations inside an IOC execution path without considering asynchronous processing.

For each benchmark, report array size, data type, algorithm, hardware, execution time, and whether allocation and EPICS overhead are included.

Do not claim hard real-time suitability without evidence.

## 8. GSL errors and resource safety

GSL's default error handler may terminate the process on certain errors.

The integration must install and document a safe error-handling strategy before executing potentially failing GSL operations.

Do not use a process-terminating GSL error handler in an IOC.

Handle GSL return codes and invalid numerical inputs.

Map computation failures to meaningful EPICS alarms/status information.

Make the error handler strategy safe when multiple operations or threads are involved.

Use RAII or equivalent deterministic cleanup for allocated resources.

Test allocation failures where practical, invalid parameters, empty arrays, and incompatible dimensions.

## 9. EPICS integration

Start with an aSub-based prototype to validate the core.

Later, evaluate:

- `numericScalar` record
- `numericArray` record
- Additional interfaces for operations requiring multiple outputs or callbacks

These dedicated record types are proposals, not existing EPICS Base record types.

Follow EPICS record support conventions, including initialization, processing, database links, alarms, registration, and resource cleanup.

Verify the exact semantics of aSub input/output buffers, `NE*` and `NEV*` counts, record processing, and output links against the supported EPICS Base version.

Do not assume linking an input waveform automatically schedules processing.

All examples must be valid for the implemented version; label conceptual examples explicitly.

## 10. Testing requirements

Every new operation must include:

1. A numerical correctness test against GSL reference behavior or an independently verified analytical result.
2. Input validation tests.
3. Edge case tests.
4. Resource lifetime tests where applicable.
5. EPICS integration tests when an EPICS-facing interface is involved.

Use appropriate absolute and relative numerical tolerances.

Test empty arrays, single-element arrays, invalid dimensions, NaN/Inf handling, and repeated execution where relevant.

Streaming tests must verify reset behavior and sample counts.

FFT tests must include known signals, forward/inverse consistency, and supported transform sizes.

Tests must not depend on Python at runtime.

CI should build and test on Linux, with additional platforms introduced incrementally.

Do not describe a test as passing unless it was actually executed.

## 11. GSL API coverage tracking

Maintain `docs/gsl-coverage.md`.

Classify GSL functionality by category:

- Statistics
- Running statistics
- Moving statistics and filters
- FFT and wavelets
- Histograms
- Interpolation
- Numerical differentiation and integration
- Linear algebra
- Special functions
- Polynomials
- Random numbers and probability distributions
- Least squares and fitting
- Roots, optimization, and ODE solvers
- Other GSL modules

Track the coverage status as:

- Not evaluated
- Planned
- Implemented
- Tested
- Unsupported, with reason

Coverage means an algorithm is accessible through a documented numeric API, not merely that the GSL library is linked.

Avoid claiming complete coverage prematurely.

## 12. Project structure

Use standard EPICS module conventions.

Suggested structure:

```text
numeric/
├── AGENTS.md
├── README.md
├── LICENSE
├── configure/
├── numericApp/
│   ├── src/
│   └── Db/
├── tests/
├── examples/
├── docs/
│   ├── architecture.md
│   ├── roadmap.md
│   └── gsl-coverage.md
└── .github/
    └── workflows/
```

Adjust the structure if EPICS build requirements justify a different layout.

## 13. Documentation

Document all public operations, input/output types, parameters, calculation conventions, error conditions, and state behavior.

For each development phase, provide:

- Implemented features
- Files changed
- Build instructions
- Test commands and results
- Known limitations
- Acceptance checklist
- Next proposed phase

Favor usable IOC examples over abstract descriptions alone.

## 14. Licensing

GSL is distributed under the GNU GPL.

Before choosing the project's license, document how linking and redistribution of GSL affect the project.

Do not automatically copy the EPICS Base license or claim that dynamically linking GSL eliminates GPL obligations.

Do not add a license file without confirming the intended licensing decision.

Keep third-party source code and licenses clearly identified.

## 15. Codex development workflow

Work in small, reviewable phases.

1. Inspect the repository before modifying files.
2. Read this AGENTS.md and relevant project documentation.
3. Identify the requested phase and its acceptance criteria.
4. Implement the smallest coherent increment.
5. Add or update tests and documentation.
6. Run all available relevant checks.
7. Report test results honestly.
8. Summarize limitations and unresolved questions.

Use feature branches and pull requests where available.

Do not merge pull requests automatically.

Do not introduce unrelated changes.

Do not silently change established interfaces.

If a task is too large, complete a coherent subset and clearly report what remains.

## 16. Development phases

- **Phase 0:** EPICS module skeleton, GSL dependency detection, build, CI, license review.
- **Phase 1:** Core registry, data model, error handling, memory management.
- **Phase 2:** Batch and streaming statistics, aSub integration.
- **Phase 3:** FFT, filtering, moving statistics.
- **Phase 4:** Dedicated EPICS scalar and array record types.
- **Phase 5:** Systematic expansion across remaining GSL modules.
- **Phase 6:** Performance optimization, portability, documentation, and release preparation.

Do not start the next phase until the current phase's acceptance criteria are satisfied or its outstanding issues are documented and approved.

## 17. Definition of done

A feature is complete only when:

- Its public interface is documented.
- The code builds in the supported environment.
- Numerical correctness is tested.
- Invalid inputs are handled safely.
- Resource ownership is correct.
- EPICS integration is tested, if applicable.
- CI checks pass or an explicit environment limitation is recorded.
- GSL coverage status is updated.
- No unrelated regressions are introduced.

The priority order is **correctness, safety, maintainability, then measured performance**. Performance remains a fundamental design objective, but unsafe or numerically incorrect optimizations are unacceptable.
