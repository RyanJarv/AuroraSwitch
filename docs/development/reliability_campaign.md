# Reliability checklist

Historical software screening covers the original seven; bounded checks also
pass for older Flux/Morse, Tempest and the experimental Fata RAM build.
Fata's real USB/wavetable path remains unverified. Physical reliability and
ordinary stock USB recovery remain open. Do not expand emulator coverage merely
to postpone those tests.

## Current source and earlier candidates

The stable-family-color change supersedes the menu colors below: all Flux
versions are yellow and all Morse versions white. It changes selector bytes,
not admission or handoff. The previously frozen candidate and virtual results
remain historical; a new release build needs exact-build physical testing.

### Previous twelve-entry build (history)

Twelve-entry development candidate, source `0657550`; real-USB manifest
`7c96a4c1fc61266dcfa78f2ff50210e6160010f7054894770f9599d0b931e2a6`;
BIN SHA-256
`0c79b24989144817dc43b2b2b7f4cc16ea2d41d93c5a302c4ebdefefeb362957`
(95972 bytes). ELF `d6c3a8937ec0221425aef2ac1687eff42d1381880dc42173bf323574cc20eeda`,
MAP `0c4a05f802b75dd7a47c4077837841b1252918d0fc59878e40c9b69e6d61406f`.
Fresh isolated packaging completed. All twelve local payloads pass the compiled
byte/staging/corruption predicate. Initial physical results are reported below;
reliability and stock recovery remain open. On 2026-10-06 it and all twelve payloads were copied to
the FAT32 test drive `AURORA MAST` (UUID `2CB7-DC92`), reauthenticated after
read-only remount, and safely unmounted. Existing settings and recovery files
were preserved; the full prior contents are backed up locally at
`.deps/usb-backup-twelve-wS2f6o/`. Only the root selector, five new payloads,
and two identity receipts changed. ST-Link was not accessed; disconnect it
before module testing because it previously caused resets.
Its synthetic-media companion is manifest `ca471d61…ca37f`, source
`2284fbe`; Fata's linked handoff/control/audio checks pass there, not on this
real-USB BIN. Prior official switching-cycle evidence remains historical.

### Initial physical screen — user reports, 2026-10-06

After the prepared-drive instructions, the user confirmed original Aurora
launched with responsive controls/audio, then confirmed FDN worked after the
requested power cycle and blue-selection launch. This supports two initial
launches and reset/re-entry between them. No independent on-device image
readback or measurement was taken. Repeated switching, new alternative images,
Fata USB loading, and ordinary stock restoration remain untested. ST-Link
disconnection was requested but has not been explicitly confirmed.

### Previous eleven-entry build (history)

Source `2270613`; real-USB manifest
`00737185bfa680f8b5e22de0677b71f6f0bf898150d8914b01b6966aad4ad4bb`;
BIN SHA-256
`8cd5ebd90708c8a7a4002bb2422c07dd931522a52f3fc15e3f09330fccf0a163`
(95852 bytes). ELF `440e5592ba83a9ade6fbb95ebaf68732ac7271ebdfe3803167c5080f32957649`,
MAP `ff0ac5f4a52421ef238d72dd26754562c4c47ed821cab6b38962c26dce9ce5a6`.
Fresh isolated packaging completed; this candidate has not been copied to a
drive, deployed or physically tested. Virtual screens use separately identified
synthetic-media companions, not this USB BIN. See [onboarding](firmware_onboarding.md).

### Previous seven-entry build (history)

Source `e983814`; real-USB manifest
`5a5a5c96814d526d0d3c7c249a71ebfd72d2c4eb62dc33b76999e62adc9b7fcf`;
BIN SHA-256
`7eeeba55132482037a3dc7aefe67cb625605fbdf30575607c8750d7e2d625155`.
See the [checkpoint](checkpoints/public_releases_20261006.md) for full identities.

On 2026-10-06 the build and seven verified payloads were copied to a test drive,
read back, and safely ejected. The previous contents were backed up. Only the
root selector, two new payloads, and build metadata changed.
Module installation and physical results remain unconfirmed.

## Preserved software evidence

- [x] Authenticate isolated builds and all seven exact payloads.
- [x] Check staged-byte revalidation, complete copy, and DMA cleanup.
- [x] Preserve repeated official switching cycles and representative stale-approval
  checks; keep staged-byte corruption rejection separate.
- [x] Complete the bounded community screens at their recorded image identities.

## Active checklist

- [ ] Seal current source; rerun affected virtual checks before physical use.
- [ ] Run the exact-build physical campaign below.
- [ ] Demonstrate ordinary stock USB recovery and close the
  [recovery release gate](recovery_release_gate.md).
- [ ] Publish results, supported hardware scope, and remaining limitations.

The [next firmware batch](firmware_onboarding.md) tracks older versions and
Tempest's bounded persistence/control/audio review and Fata's experimental RAM
build. The QSPI branch also admits Dirt Verb and HP-filter; Fata remains
RAM-only. See the [additional-image checkpoint](qspi_additional_images.md).
Their virtual checks do not extend the physical results above, and they remain
excluded from default releases. Onboarding must not delay official recovery testing.

## Short physical campaign

Keep ST-Link disconnected. Record the exact selector/payload hashes, hardware
and bootloader identity, drive/filesystem, power arrangement, and audio setup.
Start with quiet monitoring. The initial screen above does not complete the
campaign below.

1. Record normal Aurora controls/audio and the working stock updater procedure.
   Keep the original firmware and drive contents backed up.
2. Install the exact selector using the [user guide](../user_guide.md).
   Launch Aurora and check that controls change the panel and audio.
3. Reset/re-enter and test Aurora → FDN → Aurora, then FDN → Aurora → FDN.
   Record each launch, control response, audio response, and any reset failure.
4. Restore original Aurora through the normal USB updater. Compare controls,
   audio, and relevant calibrated behavior with the baseline.

A small alternative-image screen may precede stock restoration. Morse needs a
Freeze-gate clock and nonzero Mix. Sound with frozen controls is a failure.
For Fata, test the default cube without media first, then the documented USB
wavetable-loading path; a default-cube pass alone does not close its USB concern.
Check settings after Tempest/Fata switching: their records share a flash sector.

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
