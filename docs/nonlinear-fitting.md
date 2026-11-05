# Nonlinear fitting

`numeric::fitNonlinear` fits a caller-defined vector of residuals with GSL's
trust-region nonlinear least-squares solver and a finite-difference Jacobian.
The native C++ callback receives a read-only parameter vector and a writable
residual buffer, their lengths, and caller-owned context. It must write every
residual as a finite value and return true. False, exceptions, missing values or
non-finite values fail the fit. The callback and context must remain valid for
the synchronous call and may be invoked repeatedly in an unspecified order.
No callback pointer is exposed in an EPICS field.

The caller supplies a finite initial guess, 1–64 parameters, at least as many
residuals as parameters (up to 10,000), a positive tolerance applied to GSL's
three convergence tests, and a limit of 1–1,000,000 iterations. On success,
the output buffer receives the fitted parameters and a separate output receives
the sum of squared residuals. Outputs must not overlap one another, though the
parameter output may overlap the initial guess. Invalid inputs, allocation
failure, callback errors, non-finite results, or failure to converge leave
outputs unchanged. The GSL workspace is allocated for the call and freed on
return. No covariance, weights, constraints or guarantee of a global optimum
is provided. An iteration limit does not bound callback execution time.

Call `numeric::initializeGsl()` at startup. See [GSL nonlinear least squares](https://www.gnu.org/software/gsl/doc/html/nls.html).
