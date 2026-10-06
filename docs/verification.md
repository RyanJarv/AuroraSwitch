# Verification status and first physical boundary

## Documentation and isolated packaging (2026-10-06)

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

Stop before installation/testing until a recovery backup and known-good restore
path are confirmed. The first physical campaign must then verify:

1. The exact intended selector build is installed with bootloader/recovery
   preserved, and Reverse/Freeze status responds normally.
2. The real USB/FatFs path loads each exact file; missing/corrupt media fails
   visibly without launching.
3. Shift launches each image; panel controls continue responding and audio
   changes with controls, rather than merely passing sound while frozen.
4. Reset returns to the selector, and both A→B→A sequences work repeatedly.
5. Restoring the original image works using the single-drive recovery process.

Do not infer a physical pass from a green build, native tests, or initial sound.
No automatic flashing or recovery command is provided. Future broader image
support and live switching without reset need separately reviewed contracts.
