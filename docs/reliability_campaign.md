# Reliability campaign and alternative-image onboarding

Status: **bounded virtual checks complete; stop before hardware**; physical campaign
**NOT RUN**. See [the changed-build checkpoint](community_virtual_checkpoint.md).
The hard gate in [recovery_release_gate.md](recovery_release_gate.md) remains
OPEN. Catalog admission and bounded virtual results do not waive that gate for beta.

Preserved initial candidate: source `893ca5b`, manifest
`1ab83ea9407e0e53521b2dd59702f4125c6d2bbfcaf842ac82ddbd8b6b60dda1`.
See [verification.md](verification.md#frozen-initialization-error-candidate)
for historical artifact hashes and bounded software results. The newer timing
candidate below is already frozen; do not silently transfer older results to it.
The earlier official-only timing candidate is source `63d27a5`, manifest
`f8ea8d874781642b29e244cb7cce302657b80d2aeaab8cdd804b70804c45571e`.
Its evidence remains historical. The changed real-USB candidate is source
`25c393e`, manifest `e1daea377e4067f2efe96e1f426b088deb160a80e6173fe5eafa897acadd3908`.
Use its exact package for the next physical screen; identities and limitations
are in [the checkpoint](community_virtual_checkpoint.md).

## Active checklist

- [x] Freeze and authenticate the changed catalog candidate; retain the
  sealed manifest/ELF/BIN/MAP. No rebuild for documentation-only changes.
- [x] Verify fully loaded staged bytes, launch-time revalidation and exact
  copy/DMA cleanup. Keep staged-byte corruption rejection as a separate safeguard.
- [x] Complete repeated virtual Aurora → FDN → Aurora and FDN → Aurora → FDN
  control/audio/reset cycles. Reuse these results and the small stale-approval
  regression; do not start a 16-execution media-edge campaign. These full cycles
  remain bound to the earlier companion. The changed build adds repeated bounded
  official/EchoGarden/Oscillator launch/control/audio checks, not new reset cycles.
- [x] Resolve Cloudscape's control/audio comparison with an existing late-impulse
  stimulus. All three custom images pass the bounded screen; no new model.
- [ ] Run the short exact-build physical campaign below, with ST-Link
  disconnected. Stop and ask for user participation before physical actions.
- [ ] Demonstrate ordinary stock USB recovery and resolve the scoped release
  gate, then publish exact-build results and remaining limitations. Recovery is
  a requirement, not an assumed guarantee.

Whole-memory/controller or exact normal-boot state equivalence is not required.
Preserve reviewed handoff invariants; investigate more state only for a
demonstrated failure or concrete hazard. Virtual results prove neither real
USB behavior nor physical handoff/recovery. Alternative images are deferred if
they need substantial emulation/reverse engineering or delay official testing.
No new loader features, USB stack or verification framework is planned.

### Regression policy

The complete switching-cycle test is the primary integration regression. Keep
intermediate tests and historical receipts, but rerun intermediate live campaigns
only for changes affecting their code or invariants. Replay existing receipts
for verifier changes; run complete live cycles for relevant handoff/runtime
changes. Documentation-only edits need link/diff checks, not emulator campaigns.
Keep exact artifact authentication, launch-time staged-byte verification,
corruption rejection, DMA cleanup regressions and the stock-recovery release
gate fail-closed. The host test and image-authentication commands remain in
verification.md; the latest repeated cycle results are recorded there as well.

The immediate target is the two official images. Dirt Verb/QSPI support,
HP-filter staging expansion, arbitrary firmware,
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

## Short physical campaign — next user-assisted chunk

Use the changed frozen package above (BIN
`e79ed1ec20a5187f8e770a9cd77d41041f03d9deef79920f53fd5bf43782d6e7`)
and the exact official payloads in verification.md. No new build is needed for
documentation or host-test changes. Authenticate the prepared files before
installation; the currently reported responsive FDN is not exact-build evidence.

1. Record known-good normal Aurora controls/audio and the existing updater
   procedure, hardware, power and drive. Keep original firmware available.
2. Install the exact selector through the normal updater. Cold-launch Aurora;
   verify several controls change both the panel and audio, not merely sound.
3. Reset/re-enter the selector and launch FDN, then reset/re-enter and return
   to Aurora. Repeat in the opposite order (FDN → Aurora → FDN). Record each
   launch and any frozen controls, failed re-entry or audio anomaly.
4. Restore original Aurora through the ordinary USB updater using the original
   BIN as the sole root updater BIN. Verify normal controls/audio and relevant
   calibrated behavior against baseline. This is a required observation, not
   an assumption that copying a file guarantees recovery.

A small EchoGarden/Cloudscape/Oscillator launch and controls/audio screen may be
included before restoring stock. Bounded virtual passes do not physically
qualify any image. No physical action is authorized
by this document alone.

All four steps are NOT RUN against this exact candidate. Stop for user
participation before installation, moving media or pressing module controls.
No ST-Link is required or permitted in this campaign. Start monitoring quietly.
One cycle each direction is an initial screen, not a reliability guarantee or
automatic closure of every requirement in recovery_release_gate.md. Additional
persistent-state/recovery evidence required by that gate remains open; if it
requires unsafe debugger attachment, report the gap rather than claiming a pass.

## Extended official-image checklist — follow-up, not the next chunk

This preserved screening menu is optional planning context, not an instruction
to execute every row or achieve whole-state equality before proceeding. Select
extra tests only for concrete hazards/failures or remaining recovery-gate needs.

All physical rows below are NOT RUN for the frozen timing candidate. Counts are
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

The current bounded first-batch work and file-acquisition/version policy are in
[alternative_firmware.md](alternative_firmware.md). The Oscillator Is a Lie joins
EchoGarden and CloudscapeX for review; none is admitted yet. This does not waive
the official physical campaign or stock-recovery release gate below.

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

Current matching-companion handoff, connected control/audio, both reset returns
and both full switching cycles now pass repeated bounded emulator runs; see
[verification.md](verification.md). Reuse them. They are not real-USB BIN or
physical results. Omitted-Freeze executions refuse launch. Stale approval after
disconnect is an interaction-consistency safeguard, not evidence that an intact
staged RAM image is intrinsically invalid; keep its regression small.
TODO: execute the short physical campaign and independently close remaining
recovery-release obligations. No virtual result closes ordinary USB recovery.
TODO (deferred): alternative memory/persistence review and compatibility runs,
only when small and non-delaying; no new loader features by default.
Optional follow-up: measure loading latency/failure behavior before proposing timeouts/watchdogs;
synchronous upstream calls may stall, and bounded bytes are not bounded time.
Use [load_timing.md](load_timing.md) for diagnostic fields and the measurement
recipe; no real measurements have been recorded yet.
TODO: retain raw records and publish an exact-image report; do not mark planned
rows PASS or present these repetition targets as a reliability guarantee.
