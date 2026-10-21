# Testing boundaries

Run `make check` for the host suite and Python syntax checks. Run
`make test-sanitize` for the native probes under AddressSanitizer and
UndefinedBehaviorSanitizer. CI runs both commands and compiles both Arm
transports using the existing pinned Docker toolchain. A native C++17 compiler and
its sanitizer runtime are required; no module or vendor payloads are needed.

## Shared production code

`selector_controller.hpp` owns selection, discovery and automatic-launch policy.
`file_media.hpp` owns image-reader diagnostics and saved-choice file operations.
`handoff_sequence.hpp` owns validation and teardown ordering. Tests exercise
these implementations directly.

`media_transport.hpp` is the single-file I/O boundary. The firmware build links
`media_usb.cpp`; the synthetic companion links `media_virtual.cpp`. There are no
conditional test branches in the selector or handoff. Build directories and
artifact names remain separate, and synthetic bundles cannot be installed by
the USB preparation tools.

The host USB probe compiles **the production `media_usb.cpp`** against the small
SDK/FatFs declarations in `tests/fakes/`. Scripted external calls exercise open,
read, write, sync, close, readiness, initialization and unmount failures. The
probe compares file contents, full I/O call sequences, decisions and relevant
diagnostics against frozen physical method bodies from commit `9972d5d`.
Do not update that oracle to accommodate an implementation change.

Negative controls deliberately remove close, sync, CRC, readback, repeated-choice
write suppression and unmount safeguards.
Each must fail its named native assertion; a compilation failure does not count.
Saved-choice tests also exhaust 81 write cut points, 640 single-bit corruptions,
sequence ambiguity/wrap and 100,000 deterministic choices with exact write counts.
Existing probes cover staging, catalog admission, memory bounds, initialization,
DMA/handoff ordering, QSPI programming failures and control flow.

## Firmware changes

Host fakes do not execute the USB host stack, block driver or FatFs internals.
They do not prove physical DMA visibility, cache behavior, filesystem crash
consistency, timing, audio, power-on reliability or recovery. The Arm build
checks the real SDK interface and linker placement; affected linked execution
regressions check the authenticated companion. Neither replaces exact-build
hardware qualification or ordinary stock USB recovery.

Keep new tests at the smallest production boundary that can expose the failure.
Prefer explicit injected I/O outcomes and observable effects to test-only logic
inside firmware. Keep validation, launch policy and musical behavior in the guest.

## Refactor checkpoint

The [2026-10-21 checkpoint](evidence/test_refactor_20261021.json) records 76 host
tests, 13 sanitized native probes, six negative controls and fresh isolated USB
and synthetic packages. All 15 linked boot cases pass, including both modeled
Dirt/FDN three-boot handoff cycles with unchanged NOR and cleared DMA memory.
The USB build also passes its complete 20-store runtime-cleanup census; file and
filesystem objects remain 32-byte aligned in the DMA-accessible SRAM bank.

The cycles supply prepared loader context and external choice/file fixtures
after each modeled reset. They reach Dirt main or FDN reset entry, with no audio
continuation or physical reliability claim. Historical full audio-cycle records
remain bound to their older exact candidates.
