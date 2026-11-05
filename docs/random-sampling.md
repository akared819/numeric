# Random sampling

The native C++ `numeric::UniformRandomStream` generates uniform doubles in
`[0, 1)` using an independent GSL MT19937 generator. Call
`numeric::initializeGsl()` during process startup, then initialize each stream
with an explicit 32-bit seed. `fill` writes a positive number of elements to a
caller-owned buffer after checking its capacity; invalid requests do not
advance the stream. No allocation is required for repeated fills. `reset`
restarts the stream from its most recently supplied seed, and destruction
releases the generator. GSL defines the behavior of seed zero. Separate
instances have separate state; synchronize calls that share one instance.

The sequence is pseudorandom and is not suitable for cryptographic use. The
interface is native C++ only, not an IOC record operation; it does not use GSL's
environment-selected generator. See [GSL random number generation](https://www.gnu.org/software/gsl/doc/html/rng.html).
