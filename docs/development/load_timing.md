# Measure before adding load timeouts

Optional developer diagnostics, not an additional active test campaign. Use
these only to investigate a slow load or a concrete storage failure.

No physical load times have been collected. Instrumentation is diagnostic only;
it neither installs a watchdog nor turns a synchronous call into bounded work.

The exact ELF exposes two internal diagnostic symbols:

- `load_diagnostic.timing`: last file attempt, from before staging through
  close, SHA-256 and vector authentication; includes failed attempts that return.
- `discovery_timing`: whole catalog scan on media connection, including all
  files checked by that scan.

Each has unsigned 32-bit `started_ms`, `elapsed_ms`, `maximum_ms`, `completed`.
`completed` is zero until the operation returns; zero duration can mean a valid
sub-millisecond operation, not missing evidence. `maximum_ms` accumulates across
attempts during this selector session and resets on restart. Clock subtraction
supports one wrap for an operation shorter than 2^32 ms (about 49.7 days).
Guest clock timing is not independent wall time and is not a transport deadline.

Resolve addresses/field layout from the matching ELF/DWARF, not older fixed
addresses. Use read-only observation after the operation settles; snapshots
during writes can be torn. Pair `load_diagnostic` selection, attempts, bytes,
result and authentication with its time. Discovery leaves the last attempted
file's record, not a per-image history; explicitly select and Freeze each image
to measure individual successful loads. Avoid halting during a timed operation;
debugger halts perturb USB/timers and must be labeled separately.

For a timing investigation, sample successful loads and the failure case on
the affected drive. Separate first-use and repeat behavior. Also record independent
wall time/video and whether controls recover. Store observed min/median/max and
raw measurements with exact build/drive identities. Do not infer a timeout from
synthetic model speed or choose a threshold before seeing normal variation.

If a call stalls, `completed=0` identifies incomplete execution but cannot tell
its cause. Preserve the failure and determine the upstream blocking call before
adding a watchdog or asynchronous loader. Power loss/failed reads are not the
same hazard as interruption of flash programming or target settings saves.

Real measurements remain unavailable; instrumentation does not prove a maximum
wall-clock bound.
