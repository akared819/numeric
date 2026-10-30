# GSL coverage

Coverage means accessible via the documented numeric API. Status describes the implementation and test suite, not a claim that CI has passed. No EPICS-facing operations exist yet.

| Category | Status | Scope |
| --- | --- | --- |
| Statistics | Tested | `mean` only: native tests passed on macOS with GSL 2.8; Linux CI pending. All other statistics not evaluated. |
| Running statistics | Planned | Future stateful operations |
| Moving statistics and filters | Planned | Future work |
| FFT and wavelets | Planned | Future work |
| Histograms | Not evaluated | |
| Interpolation | Not evaluated | |
| Numerical differentiation and integration | Not evaluated | |
| Linear algebra | Not evaluated | |
| Special functions | Not evaluated | |
| Polynomials | Not evaluated | |
| Random numbers and probability distributions | Not evaluated | |
| Least squares and fitting | Not evaluated | |
| Roots, optimization, and ODE solvers | Not evaluated | |
| Other GSL modules | Not evaluated | |

Available statuses: Not evaluated, Planned, Implemented, Tested, Unsupported (with reason). Tested requires recorded successful verification.
