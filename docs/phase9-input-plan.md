# Phase 9: multiple input links — design under review

`numericScalar` and `numericArray` now support `INPA` through `INPP` as ordered numerical inputs. They
are separate from configuration arguments: each fills a non-parameter entry
of the operation's input signature. The existing `INP` remains available for
single-input databases. A record that configures `INP` together with any new
input link is invalid. If the new interface is used, its required links must be
contiguous and links beyond the signature must be blank.

The scalar increment covers two-array correlation and
array-plus-scalar polynomial evaluation provide representative tests. For
each input, allocate a record-owned buffer at initialization and validate the
source type, shape, valid count and capacity before executing. Waveform inputs
reserve one extra element beyond NMAX to detect truncation; scalar inputs must
contain exactly one element. Preserve the last successful output and raise an
INVALID calculation alarm if any link is invalid or disconnected. NORD retains
its existing meaning as the count read from the first input and becomes zero
on failure.

Link reads are performed during one record processing cycle, but multiple
Channel Access links do not supply an atomic cross-PV snapshot. Input changes
do not by themselves guarantee a processing event; producers must coordinate
related updates and processing. Database-link processing attributes retain
their EPICS meanings. See the official [database locking and processing
documentation](https://docs.epics-controls.org/en/latest/appdevguide/lockScanProcess.html).

The local IOC and separate-client tests cover existing INP databases, two waveforms, mixed scalar and
waveform inputs, mismatched lengths, oversized buffers, invalid types and
conflicting INP/INPA configuration. An IOC test changes the second link to an
unconnected CA PV, checks the alarm and retained output, then changes it back
to a local link and checks recovery. This does not test reconnection of an
unchanged remote link. The array increment covers two-input linear fitting,
legacy link conflict, a temporarily unconnected CA link and link restoration,
using the same input and result preservation rules. Further
acceptance includes a separate two-IOC test that stops and restarts the
producer, verifies retained outputs and alarms during disconnection, and checks
that both records use changed values after the same CA links reconnect. This
test passed locally on macOS arm64; CI validates the other platforms. Phase 9
remains in progress pending complete CI and final review.

Acceptance requires existing INP databases, two waveforms, mixed scalar and
waveform inputs, mismatched lengths, oversized buffers, invalid types,
disconnected links and recovery, alarms, and separate-client integration
tests. The record must reject conflicting INP/INPA configuration at
initialization and preserve existing output on calculation failure.
