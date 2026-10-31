# GSL coverage

Coverage means accessible via the documented numeric API. Status describes the implementation and test suite, not a claim that CI has passed. The mean operation is also exposed through the documented aSub prototype.

| Category | Status | Scope |
| --- | --- | --- |
| Statistics | Tested | `mean`, `minmax`, sample and population variance and standard deviation through core and aSub; native and IOC tests are provided. Other statistics are not evaluated. |
| Running statistics | Tested | `running_mean` and grouped `running_stats`: mean, sample variance and standard deviation, RMS, minimum, maximum and count via core and aSub, including a coordinated acknowledged stream. Other running statistics are not implemented. |
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
