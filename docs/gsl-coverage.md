# GSL coverage

Coverage means accessible via the documented numeric API. Status describes the implementation and test suite, not a claim that CI has passed. The mean operation is also exposed through the documented aSub prototype.

| Category | Status | Scope |
| --- | --- | --- |
| Statistics | Tested | `mean`, `minmax`, sample `covariance` and Pearson `correlation` (two arrays or an interleaved aSub input), sample and population variance and standard deviation through core and aSub; compatible single-input scalar-result operations also have a numericScalar record interface. Native and IOC tests are provided. Other statistics are not evaluated. |
| Running statistics | Tested | `running_mean` and grouped `running_stats`: mean, sample variance and standard deviation, RMS, minimum, maximum and count via core and aSub, including a coordinated acknowledged stream. Other running statistics are not implemented. |
| Moving statistics and filters | Tested | Centered moving mean and median, and zeroth-order Gaussian smoothing with value-padded endpoints through the native registry, aSub and numericArray; other functions and endpoint modes are not evaluated. |
| FFT and wavelets | Tested | Complex radix-2 forward/inverse and real-input radix-2 forward transforms through the native registry, aSub waveform interface and numericArray record. Other FFTs and wavelets are not evaluated. |
| Histograms | Tested | `histogram_uniform` batch counts through the native registry and numericArray record; weighted, streaming and nonuniform histograms are not evaluated. |
| Interpolation | Tested | `interp_linear` through the native registry and aSub; other interpolation functions are not evaluated. |
| Numerical differentiation and integration | Not evaluated | |
| Linear algebra | Tested | `vector_norm` (real Euclidean vector norm) through the native registry and numericScalar record; matrix operations and other BLAS functions are not evaluated. |
| Special functions | Tested | `erf` through the native registry and numericScalar record; other special functions are not evaluated. |
| Polynomials | Tested | `polynomial_eval` for real coefficients and a real evaluation point through the native registry and aSub; other polynomial functions are not evaluated. |
| Random numbers and probability distributions | Tested | `normal_cdf` (standard normal lower-tail probability) through the native registry and numericScalar record; random-number generators and other distributions are not evaluated. |
| Least squares and fitting | Not evaluated | |
| Roots, optimization, and ODE solvers | Not evaluated | |
| Other GSL modules | Tested | `log1p` from the elementary mathematical functions through the native registry and numericScalar record; other functions and modules are not evaluated. |

Available statuses: Not evaluated, Planned, Implemented, Tested, Unsupported (with reason). Tested requires recorded successful verification.
