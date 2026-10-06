# Reliability campaign and alternative-image onboarding

Status: initial software preparation complete; physical campaign **NOT RUN**.
The hard gate in [recovery_release_gate.md](recovery_release_gate.md) remains
OPEN. This plan does not admit additional firmware or waive that gate for beta.

Preserved initial candidate: source `893ca5b`, manifest
`1ab83ea9407e0e53521b2dd59702f4125c6d2bbfcaf842ac82ddbd8b6b60dda1`.
See [verification.md](verification.md#frozen-initialization-error-candidate)
for full artifact hashes and bounded software results. Timing instrumentation
now requires a new frozen package; do not silently transfer these results to it.
The current timing candidate is source `63d27a5`, manifest
`f8ea8d874781642b29e244cb7cce302657b80d2aeaab8cdd804b70804c45571e`.
Use that sealed package for hardware tests; full identities and bounded results
are in [verification.md](verification.md#current-frozen-timing-candidate).

## Scope and order

1. Freeze a clean `make package` bundle. Record source commit, manifest, ELF,
   BIN, MAP and toolchain identities. Do not test mutable incremental output as
   a release candidate. A changed BIN starts a new campaign.
2. Run the full campaign below for exact stock Aurora 1.4.4 and FDN 1.2.2.
3. Onboard EchoGarden first, then CloudscapeX, one exact image at a time. Use a
   compatibility tier rather than attempting to certify their musical behavior.
4. Publish a small report with tested hardware/updater scope, raw receipts,
   failed cases, repetitions and remaining limitations. No release until the
   recovery gate passes for every admitted image.

The proposed first-beta target is two official images and at most two community
images. Dirt Verb/QSPI support, HP-filter staging expansion, arbitrary firmware,
new bootloaders and seamless switching are excluded.

## Record before testing

Copy this checklist into a dated report; leave unknown results NOT RUN. Record
selector package path/hash, payload hashes, board/Seed markings, bootloader
identity, drive model/filesystem, power arrangement, audio recording chain and
calibration/settings baseline. Keep actual counts and raw logs/videos, not only
pass/fail summaries. Match normal and selector tests to the same payload bytes.

Hardware preparation, backups, installation and debugger observation require
their own safe execution plan. Do not interrupt programming as an exploratory
test or infer a current connection from an old capture. Begin audio monitoring
attenuated. Debugger halts can perturb timing; label them separately.

## Full official-image checklist

All rows below are NOT RUN for the new initialization-error build. Counts are
initial screening targets, not statistical reliability guarantees.

| ID | Action | Required observation | Initial repetitions |
| --- | --- | --- | --- |
| B1 | Normally install each official image | All knobs, buttons, LEDs, audio and accessible CV respond; record baseline | 2 cold boots each |
| B2 | Demonstrate original USB restore | Stock controls/audio/calibrated CV match baseline, no debug recovery | 1 before selector testing |
| M1 | Empty drive, either image alone, both images | Only exact present images selectable; no-image launch refused | 2 each |
| M2 | Wrong hash, truncated/oversized, renamed and unknown files | Invalid/unknown files omitted; failed reload cannot launch | 2 each |
| M3 | Shift before verification; change selection after verification | No launch; approval invalidated | 2 each |
| M4 | Remove media during discovery/load and after verification | No stale launch; reset/reconnect permits retry | 2 each stage |
| M5 | Replace previously discovered file with corrupt bytes, then reload | Fresh authentication fails; no launch | 2 |
| M6 | Reconnect and repeat with another compatible drive | Rescan works; record discovery/load duration and errors | 2 per drive |
| L1 | Launch each official image from cold selector boot | Controls change LEDs/audio; sound with frozen controls fails | 5 each |
| L2 | Aurora → reset → FDN → reset → Aurora; reverse order | All three launches responsive, no inherited-state regression | 10 cycles each direction |
| L3 | Run each selected image with representative controls/audio | No freeze, dropout or unexplained output; compare normal baseline | 30 minutes each |
| A1 | Record startup and handoff audio against normal boot | Report transient amplitude/noise/latency; no click-free claim | 3 each |
| R1 | Restore stock after launches and media failures | Original updater works and stock matches B1 without repair | Each failure class and both orders |
| R2 | Compare bootloader/calibration/relevant persistent regions | No unexplained protected-state change; expected target-owned changes accounted for | Before/after campaign |
| R3 | Power loss during read-only loading | Normal boot/recovery works; no retained launch approval | 2, only after B2 |

Installation interruption and target-owned settings-save interruption are
separate hazards. Review reachable writes and updater guarantees before deciding
whether/how to execute such tests; do not treat R3 as proof of either.

Initialization failure is covered by synthetic host tests for each failing step,
ordered short-circuiting and the Ready/error distinction. The firmware latches
the result before media pumping or controls; errors stay red until reset. These
tests do not inject real USB/FatFs initialization failures or prove LED hardware.

## Alternative compatibility tier

These are received-byte intake identities, not source/vendor attestations:

| Candidate | Bytes | SHA-256 | Initial stack / reset |
| --- | ---: | --- | --- |
| `AuroraEchoGarden_v0_3_1_STABLE.bin` | 94704 | `1ecb8e3efff6e5b5360cb6d6beb87455d4cd0bf58a1a26ca07d670603ebe8442` | `0x20020000` / `0x240017d1` |
| `AuroraCloudscapeX.bin` | 98968 | `564c6791142e112e2098932a185fe112999e3acf06745c1d1e5a9916a09ad8ea` | `0x20020000` / `0x24001959` |

Neither is in `firmware/images.hpp`. Size/vector checks alone are insufficient.
Local `sha256sum`, `stat` and `od -An -N8 -tx4` checks on 2026-10-06 confirmed
these received bytes still match this table; no target code was executed.
An additional offline screen of the first 166 words (the pinned STM32H750
startup table length) found 150 nonzero handler entries in each candidate;
all are Thumb addresses inside that candidate's SRAM image extent. This assumes
the pinned table layout applies; it is not independent SDK/version identification
or proof of startup, memory initialization, persistence or runtime compatibility.
Before adding each exact entry:

- Check full startup/vector and memory contract against selector staging,
  trampoline, DMA clearing and preserved clocks/MPU. Review persistence and
  explicit flash paths; record what cannot be established from binary-only input.
- Reuse existing virtual execution/handoff tests where the target can be bound
  accurately. Do not copy another image's linked addresses or construct a new
  emulator merely to claim compatibility. A model gap is an open result.
- Test normal install as a baseline, then verified selector launch, responsive
  controls/audio, five launches and two target → official → target reset cycles.
- Demonstrate normal stock USB restore and compare protected persistent state.
  Apply the same recovery gate as official images; uncertain recovery blocks
  admission/release. Target DSP bugs can be documented without certifying DSP.
- Add exact catalog/authentication tests and a distinguishable menu color only
  after reviewing the contract. Preserve the two official identities and avoid
  changing staging/handoff to accommodate a difficult image in this tranche.

Official-image results support the shared loader mechanism, not an automatic
compatibility or recovery pass for another firmware.

## Stopping rules and follow-ups

Stop on frozen controls, unexpected protected/persistent changes, unexplained
audio anomalies, indeterminate installation or failed normal restore. Preserve
the exact failure before making a fix; rerun affected rows on a new bundle.

TODO: execute exact current-build virtual handoff checks before physical tests;
older virtual evidence does not automatically qualify the new BIN.
TODO: collect physical B1–R3 results and close the recovery gate independently.
TODO: complete alternative-image memory/persistence review and compatibility runs.
TODO: measure loading latency/failure behavior before proposing timeouts/watchdogs;
synchronous upstream calls may stall, and bounded bytes are not bounded time.
Use [load_timing.md](load_timing.md) for diagnostic fields and the measurement
recipe; no real measurements have been recorded yet.
TODO: retain raw records and publish an exact-image report; do not mark planned
rows PASS or present these repetition targets as a reliability guarantee.
