# Reliability checklist

Software screening is complete for the current scope. Physical reliability and
ordinary stock USB recovery remain open. Do not expand emulator coverage merely
to postpone those tests.

## Current build

Source `e983814`; real-USB manifest
`5a5a5c96814d526d0d3c7c249a71ebfd72d2c4eb62dc33b76999e62adc9b7fcf`;
BIN SHA-256
`7eeeba55132482037a3dc7aefe67cb625605fbdf30575607c8750d7e2d625155`.
See the [checkpoint](public_release_checkpoint.md) for full identities.

On 2026-10-06 the build and seven verified payloads were copied to a test drive,
read back, and safely ejected. The previous contents were backed up. Only the
root selector, two new payloads, and build metadata changed.
Module installation and physical results remain unconfirmed.

## Active checklist

- [x] Authenticate isolated builds and all seven exact payloads.
- [x] Check staged-byte revalidation, complete copy, and DMA cleanup.
- [x] Preserve repeated official switching cycles and representative stale-approval
  checks; keep staged-byte corruption rejection separate.
- [x] Complete the bounded community screens at their recorded image identities.
- [ ] Run the exact-build physical campaign below.
- [ ] Demonstrate ordinary stock USB recovery and close the
  [recovery release gate](recovery_release_gate.md).
- [ ] Publish results, supported hardware scope, and remaining limitations.

Tempest's QSPI persistence review is deferred. Dirt Verb and oversized HP-filter
Aurora remain excluded. Alternative onboarding must not delay official testing.

## Short physical campaign

Keep ST-Link disconnected. Record the exact selector/payload hashes, hardware
and bootloader identity, drive/filesystem, power arrangement, and audio setup.
Start with quiet monitoring. All steps below are **not yet run on this build**.

1. Record normal Aurora controls/audio and the working stock updater procedure.
   Keep the original firmware and drive contents backed up.
2. Install the exact selector using the [user guide](user_guide.md).
   Launch Aurora and check that controls change the panel and audio.
3. Reset/re-enter and test Aurora → FDN → Aurora, then FDN → Aurora → FDN.
   Record each launch, control response, audio response, and any reset failure.
4. Restore original Aurora through the normal USB updater. Compare controls,
   audio, and relevant calibrated behavior with the baseline.

A small alternative-image screen may precede stock restoration. Morse needs a
Freeze-gate clock and nonzero Mix. Sound with frozen controls is a failure.

Stop on frozen controls, unexplained persistent changes, indeterminate updates,
or failed restoration. Preserve the failure before fixing it. These initial
cycles are a screen, not a reliability guarantee; remaining recovery-gate
obligations must be reviewed separately.

## Regression policy

Use complete switching cycles as the primary handoff integration test.
Rerun live intermediate checks only when affected code or invariants change.
Replay existing evidence for verifier changes. Documentation-only edits need
link and diff checks, not live emulator campaigns.

Keep exact authentication, staged-byte revalidation, corruption rejection, and
DMA cleanup checks. Do not require whole-memory or normal-boot state equality.
Investigate additional state only for a demonstrated failure or concrete hazard.

The [historical campaign](history/reliability_campaign_20261006.md) preserves
older candidates and extended test ideas. Its TODOs are not the active backlog.
