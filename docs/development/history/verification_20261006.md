# Historical record — docs/verification.md

Archived during the documentation cleanup. Status, candidates, counts, and TODOs
below describe earlier checkpoints, not current instructions. Use the
[current documentation index](../../README.md) and [verification status](../verification.md).
Original evidence and artifact identities are retained.

# Verification status and first physical boundary

## Latest changed catalog — 2026-10-06

The authoritative short status is [the community checkpoint](community_virtual_checkpoint.md):
official images, EchoGarden and Oscillator pass bounded repeated virtual
launch/control/audio checks; Cloudscape also passes after its input impulse was
moved after the observed control change. Its earlier inconclusive comparison
is preserved, not relabeled. Work stops before physical testing. The changed
real-USB candidate has fresh source/build authentication but **no physical or
stock-recovery pass**. Earlier evidence below remains tied to earlier binaries.

## Hard recovery release gate

`recovery_release_gate.md` is the mandatory acceptance condition: all supported
selector failures must be recoverable using only the original USB updater and
original firmware, with no internal backup restore, recalibration or debug tool.
This gate is OPEN and release is blocked. Historical evidence is unchanged;
none establishes an unconditional normal-restore guarantee.

## Documentation and isolated packaging (2026-10-06)

### Current frozen timing candidate

#### Staged-byte launch invariant — 2026-10-06 review

The candidate's loading/handoff sources remain unchanged from `63d27a5`.
`StageReadOnlyImage` requires the full expected length and a successful close;
the selector hashes and checks vectors in the resulting staging buffer.
`Platform::Validate` in `firmware/handoff.cpp` requires that same fixed staging
address, bounded size, SHA-256 and catalog/vector match. The shared sequence
validates before unmount and again after bus-master reset/cache teardown.
`Jump` passes that same pointer and exact image length to the SRAM4 trampoline;
it does not reopen a file. The existing complete copy/store census checks every
payload byte and target readback, with missing/extra-store rejection controls.

The host-only `authenticate_image.cpp` now additionally changes the backing
bytes after loading (the staged image still verifies), changes one staged byte
(verification fails), then restores it (verification passes), for both official
images. `python3 scripts/verify_images.py /home/me` passes on the local supplied
images; users substitute their own firmware directory. `make test` passes all
13 host tests, including validation ordering and refusal after failed final
validation. These checks add no firmware feature and do not change the frozen
candidate. They do not prove physical DMA/cache quiescence or intact RAM on
actual hardware.

The existing stale-approval rule remains: loss of media readiness withdraws UI
approval, requiring a fresh confirmation. Disconnecting media does not inherently
invalidate intact staged RAM. One small linked synthetic-readiness regression
is sufficient here; substantial disconnect/USB emulation is not a prerequisite
to the next physical campaign.

Matching-source synthetic-media discovery/loading now has bounded linked
execution evidence: both official files, seven cases each, two repetitions
(28 executions). Normal Freeze verification passes; omitted release, corrupt,
truncated, wrong-path, disconnected and absent media refuse approval and recover
after reconnect/reload. Complete guest snapshot arrays, including timing fields,
repeat exactly. These runs use a **different** virtual-transport BIN
`4d48a3a8b394a724b5973e98f9d9c17144b23173d715cebdd0509a7c7210c2d6`,
not the real-USB candidate below. It presents one file at a time and skips
USB/FatFs initialization.

Subsequent matching-companion development runs cover four linked Shift handoffs
(each official payload twice), complete selected-image/DMA-arena boundary
readbacks, 85 linked callbacks and 8,192 modeled stereo frames per run, with a
checked control change. Repetitions match; omitted/extra boundary, missing
callback and truncated-output verifier controls reject. Another repeated
development slice executes the authenticated original bootloader's timeout,
full 491,520-byte copy and real branch into this companion's reset/main and
initialized loop (`0x24001be2`, stack `0x2001ff88`). It does not use a host jump
to the initialized loop. Full register checkpoint SHA-256 is
`50842c6535b974ac2ffa839e27f4fb8c10e4dc05c025c6ae66ceb959b7de05f5`.
The cold bootloader entry is now joined to discovery, Freeze and Shift in one
process for each official payload, twice each. Two separate omitted-Freeze
executions stay Selected and refuse launch after Shift. The connected tests
halt at genuine official reset entries. Subsequent connected continuations and
both complete same-guest Aurora → FDN → Aurora / FDN → Aurora → FDN cycles
independently repeat through three startups, 85 callbacks and 8,192 frames per
phase, with changed second-image controls/audio and complete third-phase PCM.
These newer results supersede the earlier open warm-cycle boundary, not its
historical receipts. Prewrite-buffer and timer-phase differences remain explicit;
complete hardware-state isolation and timing equivalence are not established.
These externally recorded emulator results are bounded development context,
not additional claims made by this repository's host tests. The companion is
not the real-USB BIN, and its media, peripheral and cache models are approximate.
Simultaneous two-file transport/discovery, real USB and physical recovery remain
unproven. Do not rerun completed cycles merely to expand media edge coverage.

The user reports selector-launched FDN currently responds normally, but its
installed selector identity has not been authenticated. The VM-visible drive's
selector is still the earlier `a8d4663c…190cef` build, with both exact official
payloads present. Read-only inspection and safe unmount changed no drive bytes.
The user reports crashes/resets when attaching ST-Link; its cause is unresolved.
Keep it disconnected during the virtual-first tranche. This observation is not
a pass for any exact-build physical campaign row or justification to flash.

Source `63d27a5` is the current physical-campaign candidate. The clean isolated
`make package` command (same pinned PATH below) sealed manifest/package
`f8ea8d874781642b29e244cb7cce302657b80d2aeaab8cdd804b70804c45571e`.

| Artifact | SHA-256 |
| --- | --- |
| BIN (94552 bytes) | `dca22d3a9275c84d4367d1ed53fb2befbd439b171a16cb0841236fa2b95c0382` |
| ELF | `0cf1f80b4999ab292355bf13fb558081493887a3f3a7fed2fa823f14acfb1563` |
| MAP | `dc34198a442eb45f3669451661656c052d7783a245a434c9b9bc0a714dda6c26` |

Fresh bounded CPU checks consumed this exact sealed ELF/BIN. The complete
20-store cleanup census passes with inherited MPU unchanged. Both official
images pass two trampoline executions each matching the complete frozen
copy/DMA-clear records and readback. Package hashes are unchanged before/after.
The same limits below apply: no USB discovery/full startup/cycle/hardware pass.
The 13-test host suite, compile/diff checks, incremental ARM build and fresh
isolated package pass; hosted CI for implementation `63d27a5` also passed
(run `37518492274`). Real load times and all physical campaign rows are NOT RUN.

Load/discovery timing instrumentation adds last-operation, maximum and completion
fields without imposing a timeout or changing authentication/launch decisions.
The host suite passes 13 tests, including zero duration, accumulated maximum,
clock wrap and incomplete timing records. Compile checks, diff checks and the
pinned-toolchain incremental ARM build pass. See `load_timing.md`; real USB
measurements and physical tests remain NOT RUN. The older frozen package below
is preserved, not automatically the timing build's evidence authority.

The next preparation tranche adds a fail-closed initialization guard: USB,
filesystem and mount setup stop on their first error. The live loop refuses
media pumping, discovery, loading and launch after any initialization failure;
Freeze remains red until reset. A native synthetic test checks all three
failure points, call order/short-circuiting and the all-success case. The host
suite now passes 12 tests. `python3 -m compileall -q scripts tests`,
`git diff --check` and an incremental pinned-toolchain ARM build pass. The
incremental build is compilation evidence, not clean package provenance.

[reliability_campaign.md](../reliability_campaign.md) records the full official
campaign and smaller alternative compatibility tier; physical rows are NOT RUN.
EchoGarden/CloudscapeX received-byte hashes, sizes and initial vectors were
rechecked locally against the intake identities on 2026-10-06. Neither was
executed or admitted. No hardware or drive access occurred for this preparation.
The hard recovery gate remains OPEN.

### Frozen initialization-error candidate

Source `893ca5b4b8625b51d97177f2314be339a93a7bfa` was packaged with:

```sh
PATH=/home/me/opt/arm/gcc-arm-none-eabi-10-2020-q4-major/bin:/usr/bin:/bin make package
make verify-images FIRMWARE_DIR=/home/me
```

Frozen manifest/package identity:
`1ab83ea9407e0e53521b2dd59702f4125c6d2bbfcaf842ac82ddbd8b6b60dda1`.

| Artifact | SHA-256 |
| --- | --- |
| BIN (94472 bytes) | `6a14102e8d51169f4a9adda866cedf5313b528ce53568633d6dcdc32694720e7` |
| ELF | `0381917bb8b551855c7ae4e7cf2ff34b79b697d517bf2416c7e0e864fd377357` |
| MAP | `80b6a7d94838c6bbc496829b35e51c964d48d3d52b6a655ad31ca63336904eb6` |

The isolated application/library rebuild passed; both supplied official images
passed the firmware's host authentication path. Reused development CPU probes
checked this sealed ELF/BIN pair (not just the incremental build): the linked
20-store cleanup census passes with inherited MPU unchanged, and both official
images pass two trampoline executions each with byte-identical frozen complete
copy/DMA-clear store records, zero readback and unchanged source. Package bytes
were independently hashed before and after execution and remained unchanged.
The linked trampoline remains 104 bytes, SHA-256
`f37788c27c210f14e4ac4ee1b189bb446dd34b6c57883488c1e57b07aee87688`.

These are bounded synthetic CPU/register-storage checks. They do not execute
USB discovery, the entire teardown, target reset handlers, full switching cycles,
initialization failures on hardware or physical recovery. A portable standalone
recipe for the optional CPU probes remains TODO in `boot_equivalence_plan.md`.
Do not substitute this pass for campaign rows B1–R3 or the recovery gate.

The source-guided loader walkthrough and risks are in `how_it_works.md`;
the proposed normal-boot/selector comparison is in `boot_equivalence_plan.md`.
The selector's no-flash-write statement excludes installation and launched
firmware. Neither documentation nor packaging changes qualify launch behavior.

Implementation checkpoint `07858b0f9103dbea9c97bc41d5e399146bbd36da` was tested with:

```sh
make test
python3 -m compileall -q scripts tests
PATH=/home/me/opt/arm/gcc-arm-none-eabi-10-2020-q4-major/bin:/usr/bin:/bin make package
git diff --check
```

All 11 portable tests pass, including ambient-output exclusion, failed-build
and source-drift rejection, plus existing immutable-bundle/ELF-BIN controls.
The tests' build/conversion tools are synthetic; a separate real package command
rebuilt libDaisy and the selector from fresh local Git checkouts of tracked
pinned source. No old archive/object was imported. No installs or hardware
access occurred. The real command passed and sealed manifest
`9f2c5575c1e89091b6b9823355db6d6bd7f13bbcd1041c02b431384750b98ddc`.

| Artifact | SHA-256 |
| --- | --- |
| BIN (94392 bytes) | `a8d4663c5f0cd2b484006d3b026a09c820c2ef8f4f5693542be5a2a803190cef` |
| ELF | `91f460be4448a65cea0db049eef4a1f3602e9932ad157f7c9627cb16b1c20751` |
| MAP | `d9487d43a86247c839bd410541d388d6219b0e27d1b6c3da1117b6eb5a3afd86` |

The fresh BIN equals the previous discovery BIN exactly. ELF/MAP differ with
build paths; full artifact reproducibility is not claimed. The manifest records
tool hashes/versions and fixed configuration. This is clean-build traceability,
not a signed or hermetic supply-chain proof; compiler/runtime libraries and
the local build host remain trusted. Historic packages/evidence remain intact.

## Supported-image discovery (2026-10-06)

The current selector discovers only exact authenticated catalog files on media
connection. Native synthetic tests execute the shared menu helper for empty,
single and multiple entries, cycling past unavailable entries, preserving a
still-valid selection, removal, and disconnect during either an intermediate
or final probe. Existing staging tests cover read/size/close/disconnect failures;
existing image authentication and launch checks remain in use.

This discovery change has host-test and ARM-build coverage, not a new linked
USB/virtual lifecycle or physical pass. The older virtual cycles described
below apply to the preceding selector image, not automatically to this new
BIN. TODO: execute this exact new build against real FAT media with zero, one,
and both supported files, corrupt/renamed/unknown files, reconnect, failed load
and launch after discovery; repeat reset and both switching directions before
claiming end-to-end discovery/launch support.

The development selector's DMA-clean SRAM trampoline has passed complete
architectural write checks: ordered image-copy stores, all 8192 ascending zero
stores and exactly one VTOR store; full zero readback, unchanged source, exact
copy, checked target vectors/registers and finite handoff. Missing, nonzero and
extra-store execution controls reject. A bounded linked cleanup window preserves
the inherited MPU state and checks all 20 ordered stores independently.

Both supported images have independently repeated same-guest A→B→A virtual
cycles with three real linked launches, two modeled reset/bootloader returns,
changed second-image control/audio and complete cold-equivalent third startup
audio (85 callbacks, 8192 frames). The reverse cycle originally exposed retained
startup DMA data; selector-owned arena clearing fixed it without cropped audio
or host memory clearing.

Those cycles use a synthetic read-only media transport and bounded platform
models. They do **not** prove USB enumeration, FatFs transport, cache/electrical
behavior, all controls/DSP, whole-memory/controller isolation or timing
equivalence. FDN prewrite-memory and original Aurora timer differences remain
explicitly outside the bounded equivalence claim. The actual USB-transport
build has passed a fresh repeatable build, symbol/layout checks, the bounded
linked MPU cleanup window and the identical trampoline CPU checks; it has not
executed actual USB transport in that model.

`make test` runs portable synthetic host safety tests for staging failures,
handoff sequencing, backed-reader behavior and programming-target rejection.
`make verify-images` checks user-supplied exact hashes/lengths/vectors using the
same Mbed TLS/vector/authentication code as the application. Neither command
claims full virtual or physical qualification.

## Required first physical test — not yet performed

The current timing/initialization source's separately authenticated
synthetic-media companion now independently repeats both official cold-entry
routes through discovery, confirmation, launch, vendor startup and 85 callbacks
with 8192 captured frames and a checked control change. Complete ordered traces
and audio match between repetitions; omitted/extra observation controls reject.
This does not transfer the historical reset-cycle evidence to the current
discovery build, or qualify the actual USB-transport image. Both current
companion modeled reset returns now independently repeat through unchanged
retained-loader/selector copy to the initialized selector, with guest-cleared
selection and synthetic-media descriptor. This is not a complete switch cycle:
current opposite-image discovery, confirmation and launch now independently
repeat in both directions through the next image's reset entry. That image's
startup/audio now also continues through both repeated complete current
companion A→B→A cycles: three linked launches, two modeled retained-bootloader
returns, second-image changed controls and complete cold-equivalent third-entry
audio. FDN prewrite-buffer and Aurora final timer-phase differences remain
explicit limitations; whole-state/timing equivalence is not claimed. Real USB
transport and ordinary stock USB recovery remain open.

Stop before installation/testing until the user is available and recovery
preparation is confirmed. The single active checklist and short exact-build
physical campaign are in [reliability_campaign.md](../reliability_campaign.md#active-checklist).
Do not expand completed virtual campaigns or require whole-memory/controller
equivalence. Staged-byte corruption rejection remains a separate safeguard;
ordinary stock USB recovery remains unproven and required. ST-Link stays
disconnected because the user reports connection-induced resets.

Do not infer a physical pass from a green build, native tests, or initial sound.
No automatic flashing or recovery command is provided. Future broader image
support and live switching without reset need separately reviewed contracts.
