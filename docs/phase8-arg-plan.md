# Phase 8: runtime arguments — design under review

This document describes a proposed interface; it is not implemented yet.

Both dedicated record types will expose 16 writable fields, `ARGA` through
`ARGP`, ordered according to each operation's argument metadata. A blank field
selects the corresponding existing legacy field or, where no legacy field is
defined, the metadata default. A nonblank ARG field overrides the legacy field.
Fields beyond the operation's defined arguments will be rejected at processing
time if nonblank. Writing an argument alone will not request processing.

Each field is proposed as a 40-byte EPICS string so that a signed 64-bit integer
can be written in decimal without converting it through binary64. Parse the
entire string, reject non-finite floating-point values, range errors and
unrecognized Boolean or enumeration values, then apply the operation's schema
and cross-argument validation. Because EPICS can truncate DBF_STRING writes,
reject strings that fill all available character positions: this avoids
accepting a truncated numeric prefix. The limit and handling must be tested
through Channel Access as well as local database access. The legacy execution
adapter must reject integers that binary64 cannot represent exactly.

Read fields during record processing under the EPICS record lock and build a
per-cycle argument snapshot before numerical work. Separate client writes are
not a transaction: processing between writes may observe a mixture of old and
new fields. A failed validation raises an INVALID calculation alarm and retains
the previously published output. Document how to clear an override and how to
coordinate writers and processing.

The compatibility tests must cover existing databases, both scalar and array
records, empty and nonempty fields, precedence, explicit processing, valid
updates, malformed and overlong text, exact integer limits, alarms, output
retention and recovery. Include a separate CA client test; confirm no execution
occurs merely because an ARG field was written. Per-operation workspaces
remain owned by the operation and are recreated only when needed.

EPICS references: [database field definitions](https://docs.epics-controls.org/en/latest/appdevguide/databaseDefinition.html)
and [record locking and processing](https://docs.epics-controls.org/en/latest/appdevguide/lockScanProcess.html).
