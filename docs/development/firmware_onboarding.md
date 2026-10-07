# Next firmware batch

Requested scope: older supported-family releases, Tempest, then FataMorgana.
Dirt Verb stays deferred. No new image is admitted by this intake review.
Keep third-party binaries outside Git and release assets.

## Checklist

- [x] Inventory public older releases and download their exact assets locally.
- [x] Review older source build modes and explicit memory/persistence use.
- [x] Stage older exact-image entries on the development branch and extend
  catalog invariants. Linked compatibility remains unproven. Keep existing
  entries; never reuse another version's linked execution addresses.
- [x] Run older-image virtual launch/control/audio checks and affected selector
  regressions. Tempest and FataMorgana are not covered by this pass.
- [ ] Review Tempest's linked settings erase/write path before admission.
- [ ] Resolve FataMorgana's QSPI execution blocker without expanding the loader.
- [ ] Package an authenticated changed selector and stop for physical testing.

## Older public releases

Downloaded from the authors' GitHub release assets on 2026-10-06 into ignored
`.deps/older-release-intake/`. Size and first two vector words agree with the
table below. This is intake evidence, not a compatibility pass.

| Image | Bytes | Stack / reset | SHA-256 |
| --- | ---: | --- | --- |
| [Flux 0.1.0](https://github.com/DaveParr/aurora-flux-capacitor/releases/tag/v0.1.0) | 91200 | `0x20020000` / `0x24001675` | `23435b32ffe5f8d715fc459da9590b3b71bf44fbf28266aeddebb555b23b9a97` |
| [Flux 0.2.0](https://github.com/DaveParr/aurora-flux-capacitor/releases/tag/v0.2.0) | 92208 | `0x20020000` / `0x24001675` | `4ea0ab917fad8bfa225e6c95551f51f6c0817c31f7b3c82e65443e0c5e189239` |
| [Morse 0.1.0](https://github.com/DaveParr/Aurora-Morse/releases/tag/v0.1.0) | 86952 | `0x20020000` / `0x24000f4d` | `f514628388a9859334b4d9348d4e55964d46c19472152770c6afdc04bbf7269b` |

All fit existing staging. Matching reset vectors do not establish matching code
or runtime behavior. Older Discord-only images and older official releases
require an obtainable exact artifact; do not invent identities or use expiring
URLs as permanent download sources.

Development catalog entries use orange (Flux 0.1.0), violet (Flux 0.2.0), and
mint (Morse 0.1.0). Existing indices, colors, and staging capacity stay unchanged.
Do not present these as compatibility-tested or merge this branch until linked
virtual checks pass. Public release downloads still select the existing four
current payloads; older files are prepared explicitly with `prepare_payloads.py`.

Source tags resolve to Flux 0.1.0 `e0ab35abe419b87aab453ec63c542ef84e36fcd7`,
Flux 0.2.0 `e8d100f7b87768b65c1957ac1ddaab6d7845c625`, and Morse 0.1.0
`1892bafec7521b0b6dca6d8ffd209c7c3ee8808a`. All select `BOOT_SRAM` and pin
Aurora SDK `69b74a88b25e2fb4d722fc269bfd9395dd28edb5`. Their application
sources register their own audio callbacks; Flux 0.2.0 uses SDRAM delay buffers.
No explicit QSPI persistence or USB-media path appears in those application
sources. SDK initialization still loads calibration; this source review is not
a source-to-release-BIN reproducibility claim or a flash-write census.

## Tempest

Start with stable `Tempest-v1.0.0`, source commit
`1d2d9d41961bf12cbbc86637e76d4df732cd376e`, not the newer release candidate.
Its earlier intake is preserved in
[the historical record](history/alternative_firmware_20261006.md#public-release-intake--2026-10-06).

`Tempest/src/Tempest.cpp` initializes `PersistentStorage` at offset `8192`
and later saves settings. The pinned SDK places calibration at `4096`, and
its QSPI implementation erases 4-KiB sectors. Those source-level ranges are
distinct, but this alone does not prove release-BIN behavior, compatibility
with other payloads' settings, or recovery safety. Initialization can write
defaults: do not bypass it to produce a startup pass.

Next: bind the released BIN to its actual persistence call sites and check
whether existing emulation can observe the erase/write behavior. Stop if that
requires substantial new flash infrastructure. No persistence pass is claimed.

Cross-image concern: FataMorgana also uses offset `8192`, but its
`PistonSettings` layout differs from Tempest's `DistortionSettings`. The pinned
`PersistentStorage::Init` recognizes only the common FACTORY/USER marker, not
an image-specific schema. A prior record can therefore be interpreted as the
other firmware's settings; saving replaces the shared sector. This is a
source-level finding, not an observed hardware failure. Both images must not
be described as having isolated retained settings without resolving or testing
that interaction. No selector flash write or automatic clearing is added.

## FataMorgana — blocked

Reviewed source: `jfriess/Aurora-Firmwares` commit
`d5504d76370c370fdb40adcf755d8a4b9c07ee6b`.
`FataMorgana/Makefile:12` selects `BOOT_QSPI`; its preceding comment says
QSPI is required because SRAM execution has USB-host issues. This is the
author's stated constraint, not an independently reproduced finding.
The firmware loads wavetables through USB and also stores settings at `8192`.
The repository's releases currently contain Tempest assets, not FataMorgana.

The existing RAM selector cannot launch this QSPI-linked configuration.
Do not simply override the build mode and claim support: a RAM port needs
USB/DMA/memory review and testing. A flash-writing loader would change this
project's scope. Pause this image for a compatible author build or a separately
approved bounded port; it must not block the smaller older-release batch.

## Evidence boundary

No firmware, loader, supported catalog, or frozen candidate changed during
intake. No hardware was accessed. Physical handoff and ordinary stock USB
recovery remain release requirements. This checklist supplements, rather than
replaces, the [reliability checklist](reliability_campaign.md).

Intake commands: `gh api repos/<author>/<repo>/releases`,
`gh release download <tag> --repo <author>/<repo> --pattern <filename>
--dir .deps/older-release-intake`, `sha256sum`, and `od -An -tx4 -N8`.
`make check` passed all 35 host tests and Python compilation; `git diff --check`
passed. No emulator campaign was run for this documentation-only checkpoint.

## Development implementation checkpoint

Branch `codex/older-firmware-onboarding`, source
`c38592934a96f1dd09c9282a2df6497feb3a5ff4`, stages the three older entries.
`make check`: 35 tests plus Python compilation passed. Preparing all three real
assets through `prepare_payloads.py` passed the compiled firmware predicate,
including independent staging and post-load corruption rejection.
Hosted host-test run `37564301702` passed for that commit.

Fresh isolated `make package-virtual`, using GNU Arm 10-2020-q4-major, produced
manifest `a35ff9b3545c2d3203087e3f18f2235152abff24769b8e2bfd23d7898d57d3bd`:

| Artifact | Bytes | SHA-256 |
| --- | ---: | --- |
| Virtual BIN | 65484 | `e262bdcdd81d753465b64337e3391bd4ccc963423d0e3445989dbaff59ce77ce` |
| Virtual ELF | 1489896 | `30d462461b57f15d373e5b50828cc13202b62024c28d53caec77b5e3301ff17e` |
| Virtual MAP | 775809 | `1230a4080967d60ebe9a3f5fb2e5c3d55456a1ca253f7545ddb86ddd0bef05fc` |

Never install this synthetic-media build on hardware. Packaging proves build
identity, not runtime compatibility. The existing virtual campaign pins the
previous seven-image ELF/BIN/MAP, catalog count, data symbols, reset PCs, and
callback observations. New selector and per-version observation bindings are
required; do not overwrite those historical pins or substitute current-version
callback addresses. No new linked virtual pass is claimed yet.

## Older-release handoff checkpoint

The subsequent three linked executions pass discovery, verification, exact
payload copy, DMA-arena zero checks, and reset entry for all older entries.
Branch PC is `0x3800005a`; Flux reset entry is `0x24001674`, Morse
`0x24000f4c`. Full logs and receipts are frozen in the development virtualizer's
older-handoff observation archive, SHA-256
`4c9808f915182f746cdb7d1f4a95884cad150cb7326c964406eb85515a51801a`.
This supersedes only the earlier statement that no new linked pass exists.
It does not prove startup, controls/audio, USB, or physical recovery.

The existing runner now accepts only this separately pinned companion in its
onboarding lane. It rejects callback continuation until each older image's
own dispatch bindings are reviewed. Historical lanes remain unchanged and
reject onboarding receipts. Replay checks independently compare the full raw
handoff/DMA census; fail-closed controls reject changed identity/staging,
missing/extra handoff rows, and missing DMA rows. These controls are verifier
sabotages, not extra firmware runs.

Next manageable chunk: derive the older binaries' actual callback registration
and dispatch PCs, then run existing input/control/audio continuations. Do not
merge or publish the new entries as compatibility-tested before that passes.

## Older-release audio checkpoint

That subsequent chunk now passes: nine linked runs (baseline and two changed
repetitions per image), 85 complete planar callbacks and 8192 PCM frames per
run, exact input/ADC observations, finite returns, and differing baseline versus
changed PCM. The two changed runs match. Each older BIN has its own checked
registration literal and five dispatch sites; no newer image addresses are
substituted. This establishes bounded software compatibility, not hardware.

Observation archive SHA-256:
`48de31c26f0c2ad79bcd018061f9d78ab02e7b3c771e341c45a8fcd1b3b3de0a`.
Seven onboarding/replay tests and seven historical community/public tests pass.
The first Flux 0.1.0 capture failed a current-version boundary assumption:
it has an additional early ADC vector and final half-transfer/IRQ-pending flags
`0x9`, with transfer-error clear. Its exact profile now checks those observations.
The archived repeated baseline has the same ordered trace as that first capture.
No rows were dropped and no historical profile was relaxed.

Remaining software scope: Tempest's actual NOR persistence behavior and
cross-image settings policy, then FataMorgana's QSPI/USB execution blocker.
Physical testing and stock recovery remain open for the changed selector.
