# Verification status

## Current scope

Default source admits 12 RAM images; this branch's opt-in adds Dirt Verb (QSPI)
and HP-filter (RAM). QSPI Fata is retired. Physical reliability and ordinary
stock USB recovery remain open.

Frozen virtual records cover earlier exact builds, not the selector after
Fata retirement and the family-color merge. See
[onboarding](firmware_onboarding.md) and [QSPI checkpoints](qspi_additional_images.md).
Host tests cover the current source; they do not replace live or physical tests.

## Seven-image checkpoint (history)

Source `e983814`. Exact BIN/ELF/MAP and manifest hashes are in the
[checkpoint](checkpoints/public_releases_20261006.md) and its
[machine-readable record](evidence/public_releases_20261006.json).

### Software checks

- 28 host tests pass: staging, discovery, initialization errors, handoff ordering,
  payload preparation, packaging, and rejection controls.
- The compiled authenticator accepts all seven exact local payloads. Changed
  backing files cannot replace verified staged bytes; staged-byte corruption rejects.
- Official Aurora, FDN, Flux, and Morse each pass a baseline and two changed-control
  virtual runs on that companion: 12 executions, 85 balanced callbacks
  and 8192 stereo frames per execution.
- Complete payload readback, ordered copy/DMA clearing, and the real-USB build's
  bounded cleanup seam pass their checks.
- EchoGarden, Cloudscape, and Oscillator retain their
  [earlier five-image results](history/community_virtual_checkpoint.md).
- Both complete official switching cycles pass repeatedly on an earlier
  companion. They remain historical evidence for the unchanged handoff, not
  fresh reset/re-entry results for the seven-image candidate.

Virtual tests use synthetic media and approximate peripheral models. They do
not prove real USB/FatFs transport, physical handoff, every control, long-running
DSP behavior, persistence safety, or stock recovery.

### Physical boundary

That candidate and all seven authenticated payloads were staged to a
test drive, read back, and safely ejected. Installation and exact-build physical
results are not confirmed.

The next steps are in the [reliability checklist](reliability_campaign.md#active-checklist):
both official launch directions, responsive controls/audio, reset/re-entry,
and ordinary stock USB restoration. The
[recovery release gate](recovery_release_gate.md) is **open**.
ST-Link remains disconnected during that campaign because attachment caused resets.

## Reproduce local checks

```sh
make test
python3 -m compileall -q scripts tests
git diff --check
make verify-images FIRMWARE_DIR=/path/to/your/files
```

The final command needs every catalog file. To validate a subset, use the
[user guide's preparation command](../user_guide.md#supported-firmware).
Host tests use synthetic inputs; image authentication checks bytes, not DSP.
The external virtual campaign is recorded evidence, not part of `make test`;
a portable standalone replay recipe remains a follow-up.

Documentation-only changes do not invalidate the frozen BIN or require new
emulator campaigns. Firmware changes require a new authenticated package and
affected regressions; relevant handoff changes require complete switching cycles.

Earlier build identities, commands, findings, and results are preserved in the
[historical verification record](history/verification_20261006.md).
