# Phase 5 acceptance and limits

Phase 5's agreed endpoint is one documented and tested representative capability in every category in [GSL coverage](gsl-coverage.md), not comprehensive coverage of every API. The table below identifies one representative in each group; links describe the complete implemented subset and its interfaces.

| Coverage category | Representative capability |
| --- | --- |
| Statistics | [Batch mean](asub-mean.md) |
| Running statistics | [Running statistics](streaming.md) |
| Moving statistics and filters | [Moving mean](window-statistics.md) |
| FFT and wavelets | [Radix-2 FFT](fft.md) |
| Histograms | [Uniform batch histogram](histograms.md) |
| Interpolation | [Linear interpolation](interpolation.md) |
| Numerical differentiation and integration | [Polynomial Gauss–Legendre integration](integration.md) |
| Linear algebra | [Vector norm](linear-algebra.md) |
| Special functions | [Error function](special-functions.md) |
| Polynomials | [Polynomial evaluation](polynomials.md) |
| Random numbers and probability distributions | [Standard normal CDF](probability.md) |
| Least squares and fitting | [Unweighted straight-line fit](fitting.md) |
| Roots, optimization, and ODE solvers | [Bracketed polynomial root](roots.md) |
| Other GSL modules | [Elementary log1p](elementary.md) |

## Acceptance

- [x] Every category above has a documented native interface, numerical correctness and invalid-input tests, and EPICS integration tests where applicable.
- [x] GSL coverage describes the actual implemented subset and avoids claims of complete library coverage.
- [x] Public interfaces document workspace ownership, processing and error behavior.
- [x] The example IOC loads databases for the Phase 5 examples.
- [x] The final implementation PRs were reviewed and merged after both Linux CI checks passed.

## Validation evidence

The Phase 5 representative additions were reviewed and merged in PRs [#28](https://github.com/akared819/numeric/pull/28)–[#37](https://github.com/akared819/numeric/pull/37), with actual checks and limitations recorded in each PR. The final combined root-solver commit passed both Linux build-and-test runs using EPICS Base 7.0.10 and GSL; these exercised the earlier representatives as well. Local macOS arm64 tests with the supplied EPICS Base development checkout and GSL 2.8 also passed the combined interpolation, fitting, integration and root IOC tests (97 assertions). These results do not establish performance or portability beyond the tested environments.

## Scope and continuing limitations

Representative coverage does not imply implementation of every function or subcategory. In particular, random-number generation, differentiation, optimization, ODE solvers, arbitrary callbacks and many other GSL APIs remain future work. Inputs fetched from separate links require producer coordination. Some operations can allocate when an instance is initialized or its size changes, and none claims hard-real-time execution. IOC integrators must observe the process-wide GSL error-handler policy in [architecture](architecture.md). The accepted intermittent EPICS Base notification crash remains unresolved; see [Phase 2 acceptance](phase2-acceptance.md). Deployment-specific testing remains the integrator's responsibility.

The next phase is performance measurement, portability, documentation and release preparation. No release, hard-real-time guarantee or complete GSL coverage is implied by this acceptance.
