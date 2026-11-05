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
