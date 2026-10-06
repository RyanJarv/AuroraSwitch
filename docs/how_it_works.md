# How AuroraSwitch works and where its guarantees stop

AuroraSwitch is an experimental application for Aurora, not a replacement
bootloader. Its purpose is to choose between a small reviewed set of firmware
files on USB and run one from RAM. It is not seamless audio-effect switching,
an arbitrary binary loader, a security sandbox, or a general firmware updater.

## The lifecycle

1. The existing bootloader loads the installed selector into AXI SRAM at
   `0x24000000`. The selector inherits clocks, external-memory mappings and MPU
   regions. It deliberately does not initialize Aurora's normal audio/ADC or
   persistent-storage setup. See `firmware/selector.cpp`, `main()`.
2. USB host and FatFs are initialized from pinned upstream implementations.
   On media readiness/reconnect, each known catalog pathname is probed. Reads
   use `FA_READ`, exact-size checks and at most 4096 bytes per staging iteration.
   Disconnect/read/close failures reject that probe. See `UsbFileReader`,
   `AuthenticateFile()` and `support/read_only_image_staging.hpp`.
3. Each staged file must match the exact SHA-256, length, initial stack pointer
   and reset vector in `firmware/images.hpp`. Only those files enter the menu.
   Discovery does not authorize execution: it overwrites the shared staging
   buffer while checking multiple files.
4. Reverse selects an available image. Freeze reloads and verifies it, marking
   the selection Verified only on success. Reverse and observed media absence
   revoke approval. Shift requires Verified, ready media, an available selection
   and no simultaneous Reverse/Freeze edge.
5. `PrepareAndJump()` verifies the staged bytes and unmounts the filesystem.
   Failure here refuses launch before irreversible teardown. It then stops USB,
   masks interrupts, resets scoped DMA/USB/I2C/SAI peripherals and deinitializes
   the system. It clears SysTick/NVIC pending/enabled state, but preserves MPU,
   clocks and external-memory configuration. This is **not** a hardware reset.
6. It verifies the staged bytes again after cleanup and installs a tiny
   stackless trampoline in SRAM4. Failure now cannot safely restore the UI:
   `Fatal()` waits indefinitely and requires reset/recovery.
7. `firmware/copy_jump.s` copies the image over the selector in AXI SRAM, clears
   the full SDK 32-KiB DMA-buffer arena, installs the target vector table, stack
   and selected CPU control registers, and branches to the target reset handler.
   The trampoline uses no stack or calls into the application it overwrites.
8. The selected firmware owns the module. The selector does not remain resident
   to supervise it. Reset returns through the existing bootloader to the
   installed selector under the supported boot contract.

## Memory map used by the selector

| Region | Use and boundary |
| --- | --- |
| AXI SRAM, `0x24000000` | Selector execution; replaced by selected image |
| DTCM, `0x20000000` | Ordinary selector globals/stack; not USB-DMA accessible |
| D2, `[0x30000000,0x30008000)` | SDK DMA objects; cleared during terminal handoff |
| D2, `[0x30008000,0x30034680)` | 181888-byte staging buffer; not in cleared DMA arena |
| SRAM4, `[0x38000000,0x38000400)` | Reserved trampoline space |
| SDRAM/QSPI | Existing mappings inherited, not globally wiped/reconfigured |

The linker scripts assert staging/trampoline placement. These allocations and
the two image contracts are reviewed constraints, not portable addresses for
other Seed variants or modules. Memory outside the cleared/copy regions can
retain data. The target's own startup must initialize everything it relies on.

## What “authenticated” means here

SHA-256 establishes equality with locally recorded supported bytes. It does
not establish a vendor signature, source provenance of a distributed binary,
absence of bugs, or safe persistent writes. Every launched image has ordinary
privileged hardware access. The selector's read-only payload loading does not
constrain the target's USB, calibration, settings or flash behavior.

The selector installation is itself a separate persistent update. It must use
a known recovery process and preserve the existing bootloader. Do not confuse
“no flash write when selecting a payload” with “installation cannot go wrong.”

## Failure and user-risk boundaries

The non-negotiable release requirement is that supported-use failures need at
most the original USB updater/original firmware restore. Any need for debug
tools, internal backups or recalibration blocks release. This is currently
unproven, not an unconditional safety guarantee; see `recovery_release_gate.md`.

- Wrong/missing/corrupt catalog files fail closed; unknown/renamed files are
  not discovered. New image support requires reviewing layout/startup/runtime
  behavior, not just adding a hash.
- File count and read sizes are bounded, but the selector has no overarching
  wall-clock load deadline. USB/FatFs calls depend on upstream error handling;
  a bad drive can leave the UI unresponsive. No automatic recovery is promised.
- Initialization failure is latched as Error before media pumping or control
  handling. Shared host tests cover short-circuiting and the distinction from
  ordinary absent media; physical USB initialization failure is not injected.
- After irreversible teardown, a failure halts rather than retrying a partially
  destroyed runtime. Reset or power cycling may be required.
- Retained peripheral/cache/memory state can produce frozen UI or incorrect
  audio even when sound passes through. The earlier DMA-buffer issue motivates
  checking controls and startup behavior, not merely audible output.
- Audio interruption, clicks and transient output levels are not characterized.
  Attenuate monitoring during first tests. Do not describe switching as seamless.
- There is no claimed support for arbitrary firmware, QSPI-linked payloads,
  firmware beyond the staging contract, or untested hardware/bootloader variants.

## Packaging authority

`make package` requires committed clean source and authenticated pinned
dependencies. It creates fresh local Git checkouts of the exact commits,
rebuilds libDaisy and the application without copying old object/archive files,
and verifies BIN equals `objcopy` of the ELF. It records tool executable hashes,
versions and fixed configuration, checks the source did not change, then seals
the ELF/BIN/MAP and manifest in a content-addressed directory.

This improves source-to-build traceability, not behavioral qualification. It
trusts the local compiler, Git and build host. It is not a signed distribution
system or a fully hermetic/reproducible supply-chain proof. Temporary build
paths may affect ELF/MAP bytes; historical packages stay unchanged.

See `verification.md` for current evidence limits and `reliability_campaign.md`
for the active checklist. `boot_equivalence_plan.md` is historical research,
not a requirement for whole-machine equality or further emulator expansion.
