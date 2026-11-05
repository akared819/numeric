# Numerical differentiation

`numeric::differentiateCentral` in `numeric.h` invokes a caller-supplied
`double (*)(double, void*)` callback synchronously using GSL's adaptive central
difference. Call `numeric::initializeGsl()` before use. The caller owns the
callback context and must keep it valid for the entire call; independent calls
have independent callback state. The callback must be safe for any points GSL
evaluates around the requested x and may be called more than once. It must not
depend on the calling order. No callback pointer is exposed through an EPICS
field.

Provide finite x and a positive finite initial step large enough to change x in
both directions. The two distinct output references receive the derivative and
GSL's estimated absolute error on success; the estimate is not a guaranteed
bound. Invalid inputs, non-finite callback values, exceptions thrown by the
callback, non-finite results, and GSL failures return an error and leave both
outputs unchanged. Exceptions are caught before crossing into GSL. The caller
must avoid concurrent modification of its context and output variables. This
call uses no operation-owned workspace; callback execution time is under the
caller's control.

`numeric::integrateAdaptive` uses the same callback contract with GSL QAGS on
a finite, strictly increasing interval. The caller supplies nonnegative
absolute and relative tolerances and a limit of 1 to 1,000,000 subintervals;
when the absolute tolerance is zero the relative tolerance must be at least
50 machine epsilons. The workspace is allocated for the call and freed before
return. GSL failure to converge, callback failure or non-finite results return
an error and preserve both outputs. The reported absolute error is an estimate,
not a guaranteed bound. The subdivision limit does not bound the execution time
of a user callback. See [GSL adaptive integration](https://www.gnu.org/software/gsl/doc/html/integration.html).

The function is a native C++ interface, not an IOC record operation. See the
[GSL numerical differentiation documentation](https://www.gnu.org/software/gsl/doc/html/diff.html).

`numeric::findRootBrent` uses the same callback contract and GSL's bracketing
Brent solver. Supply finite increasing endpoints with opposite-sign function
values, nonnegative absolute and relative interval tolerances (at least one
positive), and an iteration limit from 1 to 1,000,000. An endpoint root is
returned directly. On convergence the distinct output references receive the
root estimate and the maximum distance to the final bracket endpoints. This is
conditional on the callback being continuous and the bracket enclosing a root;
it is not a guarantee for arbitrary callbacks. Invalid input, callback failure,
GSL failure or nonconvergence preserves both output values. The temporary GSL
solver is freed before return. See [GSL root finding](https://www.gnu.org/software/gsl/doc/html/roots.html).

`numeric::minimizeBrent` similarly finds a local minimum using GSL's Brent
minimizer. The caller supplies finite `lower < initial < upper`, with the
function at `initial` strictly lower than at both endpoints, interval
tolerances, and a bounded iteration count. Success returns the minimum's
location and the maximum distance to the final bracket endpoints. Failure or
nonconvergence leaves both outputs unchanged. The bound assumes the callback
and initial bracket meet the minimizer's mathematical assumptions; it does not
establish a global minimum. The temporary workspace is freed after the call.
See [GSL minimization](https://www.gnu.org/software/gsl/doc/html/min.html).

`numeric::solveOdeRkf45` solves a first-order system over a finite increasing
time interval using GSL's adaptive RKF45 driver. The native callback receives
time, a read-only state vector, a derivative output buffer, the dimension, and
caller-owned context. It must fill every derivative with a finite value and
return true; false, exceptions or missing/non-finite values fail the solve.
Dimensions are limited to 1–1024, the step and tolerances must be valid, and
the caller specifies a maximum of 1–1,000,000 steps. Initial and output arrays
may overlap. The solver copies the initial state, owns temporary GSL resources,
and writes the final state only after reaching the requested time; failure leaves
the output unchanged. No intermediate trajectory or stiff-system solver is
provided. A step limit does not bound time spent in the callback. See
[GSL ODEs](https://www.gnu.org/software/gsl/doc/html/ode-initval.html).
