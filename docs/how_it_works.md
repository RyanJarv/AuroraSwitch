# How it works

AuroraSwitch is a `BOOT_SRAM` application, not a bootloader replacement.
Aurora's existing updater installs and starts it; the selector then loads an
exact supported image into separate SRAM and transfers execution to it.

## Why this approach?

Supported payloads are linked to execute at `0x24000000`, the same address as
the selector. Staging them elsewhere and copying at handoff avoids relocating
their code or writing a new application to flash on each selection. The existing
updater remains unchanged.

The tradeoff is inherited state: a branch to a reset handler is not a hardware
reset. Cleanup must remove selector-owned activity while retaining the clocks,
MPU policy, and external-memory mappings expected by `BOOT_SRAM` startup.

## Memory layout

| Region | Role |
| --- | --- |
| AXI SRAM, `0x24000000` | Selector code; overwritten by the payload |
| DTCM, `0x20000000` | Globals and stack; not accessible to USB DMA |
| D2, `[0x30000000, 0x30008000)` | 32-KiB SDK DMA arena; cleared during terminal handoff |
| D2, `[0x30008000, 0x30034680)` | Current 181888-byte staging buffer |
| SRAM4, `[0x38000000, 0x38000400)` | Reserved space for the copy/jump trampoline |
| SDRAM / QSPI | Inherited mappings; not globally cleared or reconfigured |

Staging capacity is the largest catalog image rounded to 32 bytes. Linker
assertions keep it outside the DMA arena and below `0x30040000`, and pin the
trampoline to SRAM4.

## Loading and authentication

Initialization starts USB/FatFs, buttons, and LED output, but not audio, ADC, or
persistence. It skips the full hardware initializer to avoid calibration writes
and external-memory reconfiguration. Data caching stays disabled for USB-DMA
coherence.

Discovery reads catalog filenames into the reusable staging buffer. Each match
must have the exact length, SHA-256, initial stack pointer, and Thumb reset
vector recorded in [images.hpp](../firmware/images.hpp). Discovery alone does
not authorize execution.

An explicit load must read the complete file and close it successfully before
authentication. Launch rehashes that same RAM buffer; it never reopens the file
and runs a different, unchecked copy. Selection changes or media withdrawal
invalidate approval. Upstream USB/FatFs calls have no overall load deadline.

## Irreversible handoff

[PrepareAndJump](../firmware/handoff_sequence.hpp) defines the ordering:

1. Revalidate staging and unmount FatFs. Failure here refuses launch.
2. Stop USB and mask interrupts.
3. Force-reset DMA1/2, MDMA, BDMA, USB1 OTG HS, I2C1, and SAI1/2.
4. Deinitialize the Seed runtime, clean/disable caches, disable SysTick, and
   clear NVIC enables/pending bits plus pending SysTick/PendSV. Preserve MPU,
   clocks, FMC, and QSPI configuration.
5. Revalidate staging after cleanup, then copy and read back the trampoline
   in SRAM4. Failure now halts; returning to the dismantled selector is unsafe.
6. Enter the trampoline without an expected return.

The [assembly trampoline](../firmware/copy_jump.s) uses no stack, external
calls, or references to overwritten selector storage. It copies the exact
payload to AXI SRAM, zeros the DMA arena, and applies memory barriers. It sets
`VTOR` to `0x24000000`, loads MSP and the reset PC from the payload vectors,
clears `CONTROL`, `BASEPRI`, `FAULTMASK`, and PSP, then unmasks interrupts and
branches to the reset handler. An unexpected return loops in SRAM4.

The target now owns execution. This does not reproduce power-on memory or
controller state; the target must initialize state it uses. DMA clearing occurs
only in the terminal routine because earlier clearing could destroy live
selector buffers.

## Boundaries

Selector loading and handoff do not program flash. Initial installation does;
launched firmware is not sandboxed and may write persistent settings. Exact
hashes establish expected bytes, not firmware safety. Virtual switching checks
do not establish physical handoff reliability or ordinary stock recovery;
those remain hardware test requirements.

See the [source map](development/architecture.md) for implementation files.
