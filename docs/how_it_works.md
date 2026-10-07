# How it works

AuroraSwitch is an application, not a replacement bootloader. It selects an
exact supported file and starts that firmware from RAM.

## Why a RAM selector?

Aurora's existing bootloader already starts SRAM-linked applications.
Reusing it preserves the normal updater and avoids flashing each selected
payload when switching. A reviewed catalog is simpler than a general loader:
different versions and layouts do not silently inherit permission to run.

The tradeoff is the handoff. A RAM jump is not a hardware reset, so the selector
must clean up its own runtime while preserving state the target expects.
Switching restarts the firmware and interrupts audio; it is not seamless.

## Lifecycle

1. **Boot:** the existing bootloader loads the installed selector at
   `0x24000000`. Clocks, external-memory mappings, and MPU regions are inherited.
   The selector initializes buttons, LEDs, and USB/FatFs, not audio, ADC, or
   persistent-storage defaults.
2. **Discover:** probe the catalog's known paths on media connection. Read each
   file into staging and check exact length, SHA-256, stack, and reset vector.
   Only matches enter the menu. Discovery does not authorize launch because
   later probes overwrite staging.
3. **Select and verify:** Reverse chooses; Freeze reloads the selected file.
   Only a complete read, successful close, and authentication mark it Verified.
   Selection changes or media absence revoke that approval.
4. **Prepare:** Shift requires a verified selection and ready media. The handoff
   rechecks the same staged bytes, unmounts/stops USB, masks interrupts, resets
   scoped DMA/USB/I2C/SAI peripherals, and deinitializes the runtime.
   SysTick and NVIC enabled/pending state are cleared.
5. **Final check:** verify staged bytes again after cleanup and copy a tiny
   trampoline to SRAM4. Failure before teardown refuses launch; failure after
   irreversible teardown halts and requires reset.
6. **Replace and start:** the stackless trampoline copies the payload over the
   selector, clears the SDK's full 32-KiB DMA arena, installs the target vector
   table and stack, resets selected CPU control registers, enables interrupts,
   and branches to the target reset handler.

AuroraSwitch is gone after launch. The payload owns the module. Reset returns
through the existing bootloader; there is no resident supervisor.

## Memory layout

| Region | Purpose |
| --- | --- |
| AXI SRAM, `0x24000000` | Selector execution; overwritten by the payload |
| DTCM, `0x20000000` | Globals and stack; inaccessible to USB DMA |
| D2, `[0x30000000,0x30008000)` | SDK DMA arena; cleared at terminal handoff |
| D2, `[0x30008000,0x30034680)` | 181888-byte staging buffer; outside the cleared arena |
| SRAM4, `[0x38000000,0x38000400)` | Reserved trampoline space |
| SDRAM/QSPI | Bootloader mappings retained; not globally wiped or reconfigured |

The trampoline must run elsewhere because it overwrites the selector's code.
It uses no stack or calls into that code. Linker assertions enforce placement.

MPU regions, clocks, and FMC/QSPI configuration are retained for `BOOT_SRAM`
startup. Unused RAM may retain data; targets must initialize what they read.
The selector disables data caching for USB-DMA coherence and tears down caches
before copying. Real cache/DMA timing still needs physical testing.

## Checks and limits

SHA-256 means equality with recorded bytes, not an author signature or safety
certification. Loading and handoff do not program internal flash or QSPI.
Installing the selector is a persistent update; a launched payload has normal
hardware access and may write its own settings or flash.

The loader reuses pinned Aurora SDK/libDaisy USB/FatFs and Mbed TLS SHA-256.
It has no overall load deadline; upstream storage calls can stall.
Audio transients, hardware variants, long-run stability, and physical recovery
remain unverified. QSPI-linked and oversized payloads are unsupported.

Ordinary stock USB restoration is a
[release requirement](recovery_release_gate.md), not a proven guarantee.
See [verification status](verification.md), the [source map](architecture.md),
and the [development guide](development.md).
