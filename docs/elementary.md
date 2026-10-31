# Elementary mathematical functions

`log1p` evaluates the natural logarithm of `1+x` using GSL's [`gsl_log1p`](https://www.gnu.org/software/gsl/doc/html/math.html). This API is intended to retain accuracy for small `x` where forming `1+x` first can lose precision. Its input and output are one DOUBLE scalar each; input must be finite and strictly greater than -1. For example, `log1p(0)=0` and `log1p(1)=ln(2)`.

Invalid descriptors, counts, buffers, NaN, infinity and inputs at or below -1 return `InvalidArgument`. A non-finite GSL result returns `NumericalError`. Failure clears the output count and preserves the previous output value. Floating-point rounding and underflow are not separately diagnosed. The adapter owns no workspace or random state and allocates nothing during execution. Initialize the non-terminating GSL policy as described in [architecture](architecture.md#gsl-setup-for-an-ioc).

The [numericScalar record](numeric-scalar.md) exposes OP `log1p` with a scalar DOUBLE input; more than one element is rejected. In the standalone IOC, `demo:logOnePlus` reads `demo:erfInput`. Write the input, explicitly process the record, and inspect VAL and SEVR. Failed calculations preserve VAL, clear NORD and raise CALC/INVALID.
