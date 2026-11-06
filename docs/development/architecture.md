# Architecture

## Source map

See [How it works](../how_it_works.md) for the design and memory contract.

| Source | Responsibility |
| --- | --- |
| `firmware/images.hpp` | Exact image identities, launch methods and shared presentation metadata |
| `firmware/selector.cpp` | Minimal hardware initialization, USB host, panel and authentication |
| `firmware/selector_controller.hpp` | Startup, one-shot automatic launch and explicit retries |
| `firmware/usb_files.hpp` | Concrete FatFs discovery, bounded reads and checked choice-file saves |
| `support/usb_choice.hpp` | Bounded, editable filename choice format, independent of menu positions |
| `firmware/handoff_sequence.hpp` | Validation and cleanup ordering |
| `firmware/handoff.cpp` | Peripheral cleanup, staged-byte rechecks and trampoline validation |
| `firmware/copy_jump.s` | Stackless RAM copy, DMA clearing, VTOR/MSP and terminal jump |
| `support/selector_constants.h` | Shared hardware bounds and binary-format constants |
| `scripts/package.py`, `scripts/sealed_bundle.py` | Reproducible selector-only build and authentication |

## Memory and startup contract

The stock loader installs the selector at `0x90040000` and copies it to
`0x24000000` at boot. Read-only access to the installed image supplies the exact
selector backup; live RAM contains globals changed by startup.

| Region | Role |
| --- | --- |
| `0x24000000` | Selector, then the chosen RAM firmware |
| `0x30000000`–`0x30008000` | DMA-accessible USB/FatFs and LED buffers |
| `0x30008000`–`0x30040000` | Staged original image |
| `0x38000000`–`0x38000400` | Stackless terminal copy/jump routine |

The linker reserves staging and SRAM4. USB DMA cannot reach DTCM, so all
filesystem transfers use the DMA arena or staging. Native tests supply the
FatFs boundary; there are no alternate transport macros or firmware test modes.

Handoff masks interrupts, resets memory-writing DMA/USB/peripherals, disables
runtime/cache state, reauthenticates staged bytes, and copies/verifies the SRAM4
routine. That routine replaces AXI code, clears the DMA arena, sets VTOR and MSP,
then enters the original reset handler. A post-teardown failure is terminal.

Preserve the bootloader's clocks and MPU: BOOT_SRAM startup deliberately inherits
them. The DMA clear removes selector buffers, not all stock RAM state. Supported
originals copy initialized globals and clear BSS themselves. Other RAM, including
`NOLOAD` regions, can retain earlier contents. The contract is vendor startup,
not exact reproduction of every register and RAM byte after power-on.

## Saved choice

`0:/aurora/choice.txt` contains one exact RAM catalog filename, optionally followed
by LF or CRLF. The read buffer is sized from the longest catalog filename plus
CRLF; empty/oversized files, unknown names, extra bytes and installation entries
provide no automatic launch authority. There is no checksum for users to maintain.
The actual firmware is still authenticated by its full hash, size and vectors.

Read the choice after the first successful scan. Any held startup button or an
explicit menu action cancels automatic boot. Reinsertion after an attempted boot
does not retry it. Dirt installation entries are excluded.

Before a manual save, reread the record. If it already identifies that image,
never open it for writing. Otherwise replace it once and check full write, sync,
close and validated readback. Failure warns and allows that session's checked
launch; there is no background retry. A torn save can lose the remembered choice.
No on-module journal or recovery copy is needed for this acceptable loss.

## Dirt USB operation

Two separate Freeze presses authenticate and confirm. Root-directory checks
reject unrelated BINs, case-insensitive `.bin` substrings, short-name aliases and
hidden installers. An existing `AuroraSwitch.bin` must match the entire installed
selector byte-for-byte; its extent comes from the pinned linker's initialized
`.data` load extent, not the running RAM image.

After confirmation, rename that exact installer to `AuroraSwitch.disabled`.
If it was removed, create the backup from the installed image. Existing backup
and return-text files must match exactly and be visible; never overwrite them.
Write `RETURN TO AURORASWITCH.txt`, then reload/authenticate Dirt because backup
checks use staging as scratch. Create root `DIRTVERB.BIN` from those bytes, retain
the original in `aurora/`, and authenticate the complete destination again.

Check that Dirt is the sole root BIN, close files/directories, unmount, stop USB
and request ordinary `NVIC_SystemReset()`. Do not manipulate the hardware boot
pin. Dirt never uses direct flash handoff or selector flash programming.

Rename synchronizes metadata; new files check writes, sync, close and full
readback. Unmount alone does not flush. The pinned disk driver's `CTRL_SYNC`
does not issue a device cache flush, so checked results do not establish
power-loss durability. After entering the confirmed operation, any failure is
terminal for writes: no reset, rollback or retry, including after USB reinsertion.

## Design boundaries

Keeping vendor images or a journal in unused-looking flash assumes ownership we
have not established: firmware may use that space for settings or other data.
Dirt also requires the stock application flash entry; coexistence needed patched
vectors and a resident wrapper. Compression solves neither problem. USB RAM
loading and ordinary replacement for Dirt avoid those layout assumptions.

There are no installed vendor slots, resident wrapper, flash journal or selector
flash-write driver. The selector and vendor both run privileged. Supported
firmware must still preserve the selector's application area; RAM execution and
small binaries do not prove that. Compare harmful persistent effects against the
same exact firmware running stock, as specified in the
[failure-mode plan](failure_modes.md). Physical qualification remains separate.
