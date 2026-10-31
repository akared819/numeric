# Proposed acknowledged streaming transport

This document is a design proposal, not an implemented interface. The current aSub routines read sample and sequence from different links and do not provide lossless delivery. Sequence-gap detection can reveal some missed updates, but cannot reconstruct them or prove that the value and sequence belong together.

An initial bounded transport could use a single coordinated producer and one in-flight event per stream:

1. The producer submits one atomic payload containing a stream/session identifier, sequence number and sample. The payload must be read as one value, not from separately updated fields. The initial representation and exact numeric bounds must be specified before implementation.
2. The consumer accepts only the next sequence for that session. It validates the entire payload before updating its per-stream workspace. An invalid event is rejected without advancing the acknowledgment.
3. After processing, the consumer retains an acknowledgment containing the session and sequence and the associated result/status. The producer waits for this acknowledgment before submitting another event. Repeated delivery of an already accepted event returns the retained acknowledgment without applying the sample again.
4. A full in-flight slot produces an explicit busy result; events are never silently overwritten. The producer must retry or apply backpressure. Any queue larger than one must specify capacity, ordering and overflow behavior explicitly.
5. A reset is an ordered event with an acknowledgment. Session changes and sequence rollover require a defined handshake. Restarting either process must invalidate or reconcile the previous session; in-memory state alone cannot promise exactly-once processing across an IOC crash.

Channel Access puts, monitors and record processing have different completion semantics. Implementation must verify which completion and acknowledgment can be observed by the producer, handle disconnection and retry, and must not use monitor delivery as proof that every event was processed. The protocol only guarantees no loss within its stated connected session when the producer obeys the acknowledgment and retry rules. It cannot recover samples the producer discards before acknowledgment or guarantee persistence across crashes without additional durable storage.

Acceptance before describing this as implemented: test repeated events and acknowledgments, full-slot behavior, sequence gaps, invalid samples, reset ordering, producer/consumer disconnect and reconnect, IOC restart, and destruction. Exercise concurrent producers or reject them explicitly. Verify atomic payload reads and output acknowledgments with EPICS Base 7 in an IOC, including CA behavior. Document buffer ownership, per-stream synchronization, count limits, timeout and backpressure policies, and numerical failure recovery.
