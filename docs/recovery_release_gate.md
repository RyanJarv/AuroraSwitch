# Hard release gate: normal USB restore is sufficient

Status: **OPEN / RELEASE BLOCKED**. No current host tests, virtual switching
results or successful build prove this requirement. It applies to experimental
as well as non-experimental releases; labeling a build experimental is not a
substitute for closing the gate.

## Required outcome

For the explicitly supported hardware/bootloader revisions and exact firmware
catalog, the worst permitted consequence of selector installation or operation
is a malfunction recoverable through Aurora's original USB updater:

1. Prepare a compatible USB drive with the known-good original Aurora BIN as
   the only updater BIN at its root; remove selector/other root updater BINs.
   Unrelated files need not be deleted unless the documented updater requires it.
2. Enter the original updater using its documented procedure. Reset or power
   cycling and normal panel controls are acceptable recovery steps.
3. Restore original firmware and verify normal panel, audio and calibrated CV
   behavior against the pre-test baseline.

Copying the file does not itself run the updater. Public recovery instructions
must explain the complete tested procedure, without claiming automatic recovery
merely because a USB drive contains a BIN.

## Release-blocking outcomes

- The original updater cannot be entered or cannot restore the original image.
- Recovery requires ST-Link, ROM DFU, soldering/opening the module, service,
  internal-memory restoration or replacing hardware.
- Calibration or other persistent corruption leaves stock operation abnormal
  after the normal restore, or requires recalibration/internal backup recovery.
- Selector-induced hardware/output damage cannot be remedied by that restore.
- A supported path can alter the recovery bootloader or protection configuration.
- An unexplained relevant persistent change or incomplete recovery result leaves
  the requirement uncertain. “Probably recoverable” is not a closed gate.

Engineering backups/debug tools remain useful during development, but recovery
using them does **not** count as a pass. Preserve a counterexample and fix its
cause (or remove the affected image from support and revalidate the remaining
scope); do not hide it behind a disclaimer or accepted divergence.

## Evidence required before release

- Bind installation, switching and recovery results to exact selector and
  payload hashes, hardware/bootloader identity and updater procedure.
- Review linked selector writes and target persistence/initialization paths.
  Ordinary RAM writes are not flash erase/program operations; the concern is
  reachable explicit flash operations or corrupted execution reaching them.
- Independently compare bootloader, calibration and relevant persistent regions
  before/after tests. Explain any expected target-owned changes and demonstrate
  that normal restore still restores proper operation without special repair.
- Demonstrate normal USB restore after successful launches and bounded failed
  launches, media/read failures and both reset/switching directions. Cover
  interrupted loading and assess installation/target-persistence interruption
  separately; do not assume these have the same recovery boundary.
- Verify post-restore controls, audio and calibrated CV behavior, not simply that
  stock boots or produces sound. Publish the exact supported scope and results.

These checks establish a scoped engineering case, not a mathematical guarantee
against every conceivable failure. Existing physical damage, unrelated hardware
faults, unsupported payloads and misuse are not covered by this project claim;
failures caused by the supported selector workflow are not excluded merely to
make the gate pass. Any unresolved gap within the intended scope blocks release.

TODO: collect exact-image physical recovery/persistent-region evidence and
review it against this gate. Do not change OPEN to PASS from software tests alone.
