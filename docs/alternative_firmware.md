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

Initial intake inspected the received files offline. The subsequent linked
catalog build is being validated virtually; nothing has been installed on a module.

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
  software results do not close the [release gate](recovery_release_gate.md).

The historical virtual continuation evidence binds to the two official images.
Custom-image execution needs its own reviewed linked addresses and input/audio
observations; copying official PCs would be false evidence. The minimum missing
capability is an exact-image startup/continuation binding, not another USB stack
or emulator. No new loader features or broad framework are authorized here.
An ELF/MAP or source from an author could simplify that work, but is not
assumed available. The authenticated changed-build observations and explicit
Cloudscape timing correction are now recorded in
[the current checkpoint](community_virtual_checkpoint.md). Do not transfer
historical results or infer a full community-image pass.

Dirt Verb and other QSPI-linked images remain deferred. HP-filter Aurora remains
excluded. Official-image reliability and ordinary stock recovery remain open
under the [single reliability checklist](reliability_campaign.md#active-checklist).

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
