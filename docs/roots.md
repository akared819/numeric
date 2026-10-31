# Bracketed polynomial root

`polynomial_root` finds one real root using [GSL's Brent solver](https://www.gnu.org/software/gsl/doc/html/roots.html). The native inputs are two to 64 finite DOUBLE coefficients in ascending power order and finite scalar lower and upper bounds. Bounds must be strictly increasing with a finite difference. Unless an endpoint evaluates to zero, polynomial values at the endpoints must have opposite signs. A bracket can contain multiple roots; the interface does not choose a particular one or find all roots. Repeated roots without a sign change may not be found.

The solver performs at most 128 iterations and tests the bracket with absolute and relative tolerances of `1e-12` using GSL's interval test. Non-convergence, allocation failure, non-finite polynomial evaluations or non-finite solver state are errors. Floating-point evaluation and conditioning limit accuracy; convergence is an interval criterion, not a guarantee on residual error. The previous output is not modified on failure.

Each operation instance owns and reuses a GSL solver, allocated on first use and released on reset or destruction. Do not execute the same instance concurrently. The polynomial callback evaluates at most 64 coefficients and solver work is bounded, but first-use allocation and synchronous IOC processing mean this is not a hard-real-time interface.

`numericPolynomialRoot` uses DOUBLE aSub input A for coefficients, B and C for lower and upper bounds, and VALA for the root. Reserve an extra element in NOA beyond the maximum coefficient waveform to detect potential truncation. Both bound links and the output link must be scalar DOUBLE. The example in `examples/roots.db` supports four coefficients. Explicit processing is required. Inputs are fetched separately; producers must coordinate updates. Failure raises CALC/INVALID and preserves the previous result. See [architecture](architecture.md) for IOC initialization and the GSL error-handler policy.

Only bracketed polynomial roots are supported; arbitrary callbacks, optimization and ODE solvers are not implemented by this interface.
