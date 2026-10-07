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
- [ ] Run existing virtual launch/control/audio checks on each new image and
  affected selector regressions. Record exact identities and limitations.
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
