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

The function is a native C++ interface, not an IOC record operation. See the
[GSL numerical differentiation documentation](https://www.gnu.org/software/gsl/doc/html/diff.html).
