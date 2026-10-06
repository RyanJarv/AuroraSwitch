# Verification status and first physical boundary

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
