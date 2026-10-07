# Historical record — docs/alternative_firmware.md

Archived during the documentation cleanup. Status, candidates, counts, and TODOs
below describe earlier checkpoints, not current instructions. Use the
[current documentation index](../README.md) and [verification status](../verification.md).
Original evidence and artifact identities are retained.

# Alternative firmware: bounded onboarding

## Distribution and versions

Keep third-party BINs out of Git, GitHub release assets and selector packages.
Users obtain them from their authors. `scripts/prepare_payloads.py` prepares only
exact images in the compiled launch catalog; it neither downloads nor installs
firmware. File authenticity here means expected bytes, not an author signature
or a firmware-safety certification.

The compiled `firmware/images.hpp` remains the one launch authority. The host
checker exports its metadata rather than maintaining another allowlist.
Preparation uses private copies and validates all inputs before publishing a
new output directory. It never overwrites an existing directory, reopens a
different source for copying, or puts payloads at the USB updater root.

Follow-ups, after the first compatible additions:

- Optional explicit fetching from an author's pinned HTTPS release asset,
  with exact size/hash checks, bounded download size/time and atomic publication.
  Never resolve “latest”; URL failure or changed bytes must fail closed.
- Manual fallback for unavailable URLs and Discord-only files; do not depend on
  expiring attachment links or mirror author binaries in our release assets.
- Older versions as distinct catalog entries, filenames and menu identities.
  Reuse an entry for identical bytes; a version label alone never admits changed
  bytes. Preserve previously supported identities when adding versions.

These follow-ups need no networking inside the selector. A user-provided file
must pass the same checks as a future automatically fetched file.

## First batch — 2026-10-06

Historical first batch: offline intake and bounded linked virtual checks are
complete. The exact five-image results remain at their original identities;
they do not qualify physical use or the newer selector build.

| Image | Bytes | SHA-256 | Stack / reset |
| --- | ---: | --- | --- |
| EchoGarden v0.3.1 | 94704 | `1ecb8e3efff6e5b5360cb6d6beb87455d4cd0bf58a1a26ca07d670603ebe8442` | `0x20020000` / `0x240017d1` |
| CloudscapeX | 98968 | `564c6791142e112e2098932a185fe112999e3acf06745c1d1e5a9916a09ad8ea` | `0x20020000` / `0x24001959` |
| The Oscillator Is a Lie v0.0.2 | 103604 | `5279d277c61d3d3693b94171bda5d48124e7529920f4ef422f7ff2812bb5c600` | `0x20020000` / `0x24000959` |

All fit the current 181888-byte staging buffer. Under the hypothesis of the
pinned SDK's 166-word startup table, each has 150 nonzero handler entries, all
Thumb addresses inside its own SRAM image. This is a structural screen only:
it establishes neither SDK identity, memory initialization, persistence safety,
startup success nor compatible handoff.

## Short active checklist

This checklist describes the completed first batch. The additional public
release intake below has its own status; earlier evidence is not proof for a
changed selector catalog.

- [x] Offline manual preparation using the existing compiled catalog and exact
  authentication; no binary redistribution or parallel allowlist.
- [x] Reconfirm received hashes, sizes and startup-table structural bounds for
  the first three candidates.
- [x] Review all three reset/data/BSS/main/StartAudio paths and bind their own
  callback/control addresses, without substituting official-image PCs.
- [x] Add exact catalog entries and distinct menu colors. Preserve staging size,
  handoff, launch-time revalidation and DMA cleanup unchanged.
- [x] Finish bounded repeated launch/control/audio checks for all three custom
  images. Cloudscape needed only a late input impulse after the control change;
  preserve its earlier inconclusive comparison as history.
- [x] Authenticate fresh real-USB and synthetic-media builds; rerun affected
  cleanup/copy/DMA checks, official bounded launch/audio checks and preserved
  primary switching-cycle replay. No new-catalog reset-cycle pass is claimed.
- [x] Resolve Cloudscape's bounded comparison without a new model or producer
  workaround. No broader reverse-engineering tranche is needed for this screen.
- [ ] User-assisted physical compatibility checks and ordinary stock recovery;
  software results do not close the [release gate](../recovery_release_gate.md).

The historical virtual continuation evidence binds to the two official images.
Custom-image execution needs its own reviewed linked addresses and input/audio
observations; copying official PCs would be false evidence. The minimum missing
capability is an exact-image startup/continuation binding, not another USB stack
or emulator. No new loader features or broad framework are authorized here.
An ELF/MAP or source from an author could simplify that work, but is not
assumed available. The authenticated changed-build observations and explicit
Cloudscape timing correction are now recorded in
[the current checkpoint](../community_virtual_checkpoint.md). Do not transfer
historical results or infer a full community-image pass.

Dirt Verb and other QSPI-linked images remain deferred. HP-filter Aurora remains
excluded. Official-image reliability and ordinary stock recovery remain open
under the [single reliability checklist](../reliability_campaign.md#active-checklist).

## Public-release intake — 2026-10-06

Morse v0.2.0 and Flux Capacitor v0.3.0 fit the existing staging allocation
and use the same pinned Aurora SDK source revision as the selector. Their
release BINs, not newly compiled substitutes, are the exact catalog inputs.
Public source is useful review context; no independent source-to-release-BIN
reproducibility claim is made. The selector/handoff/DMA mechanism is unchanged.
Their repeated virtual compatibility screen **passes**, physical testing **open**.
See [the exact-build checkpoint](../public_release_checkpoint.md) for limits and hashes.

| Image | Release URL | Bytes | SHA-256 | Stack / reset |
| --- | --- | ---: | --- | --- |
| Flux Capacitor v0.3.0 | [author release](https://github.com/DaveParr/aurora-flux-capacitor/releases/tag/v0.3.0) | 92936 | `383f0fbdca991d939c4b35cd1e2f68504d573c2416e2c798b8a699ea3cf990f3` | `0x20020000` / `0x24001675` |
| Morse v0.2.0 | [author release](https://github.com/DaveParr/Aurora-Morse/releases/tag/v0.2.0) | 88264 | `001ac1ffd668fc29f5a936b502f5235e196671caf4f21924f04aa71d99d4d9d1` | `0x20020000` / `0x24000f4d` |

Download those exact assets from the authors (for example with `gh release
download v0.3.0 --repo DaveParr/aurora-flux-capacitor --pattern
'flux-capacitor-0.3.0.bin' --dir local-firmware`). Then use the existing
`prepare_payloads.py` with whichever supported files you own. No downloading
inside the firmware or additional network framework is needed.

Morse requires an external Freeze-gate clock; silence without that input is
expected, not a failed loader. The virtual screen must stimulate the existing
GPIO model rather than manufacture a host envelope. Flux requires its own
callback/control address review, not official-image address substitution.

Tempest v1.0.0 was screened, but is **not admitted**. Its 109872-byte SRAM BIN
(`6fdb962135b2784813523a70e73bd1644b1d5b1e6637bb042c714dd7830931eb`)
fits, with stack/reset `0x20020000` / `0x240033f5`. Unlike Morse/Flux, its
source initializes `PersistentStorage` at QSPI offset `8192` and saves
settings. Before onboarding, review that linked persistence path and its
compatibility with the installed selector/updater/settings; determine whether
existing flash modeling suffices. Do not skip initialization, fabricate a save,
or expand the emulator merely to claim support. Source reviewed at release tag
`Tempest-v1.0.0`, commit `1d2d9d41961bf12cbbc86637e76d4df732cd376e`.
The newer release candidate is deliberately not substituted for that stable tag.
TODO: bounded Tempest persistence/recovery review after the two smaller additions.

Morse tag commit: `7e702fdd92c7963047f6857a122c35b8db11d22a`.
Flux source commit: `eed85107f9cc1df3c9ef93d478fc45d5d9f9b882`
(the annotated `v0.3.0` tag object is `4e346a8a54120bd58ddbe0a35f7af0ab0d67f5ba`).
Both pin Aurora-SDK `69b74a88b25e2fb4d722fc269bfd9395dd28edb5`.
Locally downloaded asset sizes and SHA-256 agree with GitHub release metadata;
that is a byte-identity check, not a signature or safety certification.

## Preparation checks

Host tests cover exact files/subsets, unsupported or changed bytes, size limits,
missing inputs, duplicate selections, rejected C++ authentication, changed source
files after validation, output/symlink refusal and malformed catalog metadata.
They use synthetic files and a mocked authenticator, not DSP execution.
Real official-image checks additionally compile and execute the actual C++
authenticator, including its staged-byte mutation rejection control.

Commands used for this checkpoint:

```sh
make test
python3 -m compileall -q scripts tests
git diff --check
python3 scripts/verify_images.py /path/to/official-files
python3 scripts/prepare_payloads.py --output /path/to/new-local-folder \
  /path/to/official-files/AR_FDN_v1_2_2.bin \
  /path/to/official-files/Aurora_v1_4_4.bin
```

No selector-linked source changed in the earlier preparation-only chunk
(`28d21cd`). The later catalog additions change the selector image, not its
handoff, DMA-cleanup or staging memory contract. The host-only `authenticate_image.cpp` is not linked into
the selector. The existing frozen physical candidate remains source `63d27a5`
with its original manifest and evidence; it is not relabeled as a new build.
