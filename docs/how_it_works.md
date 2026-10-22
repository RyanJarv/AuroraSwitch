# How it works

Only AuroraSwitch is installed at the stock application's flash address,
`0x90040000`. The stock bootloader copies this RAM-linked selector to
`0x24000000` at startup. Supported original firmware stays unchanged in
`aurora/` on USB; the selector never programs or erases on-module flash,
initializes persistent settings there, or changes flash protection.

The selector scans catalog filenames and sizes when USB becomes available.
Presence is not authentication. On launch it reads the chosen file into separate
staging RAM, authenticates its complete hash, size and vectors, and rechecks the
actual staged bytes during handoff. Unsupported or flash-running images cannot
launch. Dirt Verb is deliberately excluded.

## Remembered choice

`aurora/choice.dat` contains a version marker, exact catalog image hash and CRC.
A valid saved image launches automatically once at startup when USB is available.
Hold Shift + Reverse during power-on to open the menu. Invalid/missing settings,
missing firmware or unavailable USB leave a menu/error state. Reinsertion scans
again; Freeze provides an explicit retry. Failures never cause automatic load or
save loops or fallback to a different image.

Only a changed, authenticated manual selection writes the choice file. Repeat
choices and automatic boots do not write it. Saves check write, sync, close and
readback. A failed save warns briefly, then permits the verified session launch.
A torn replacement can lose the choice; next boot opens the menu. The file is
not a transactional filesystem guarantee, so keep the USB backed up.

## RAM handoff

| Region | Role |
| --- | --- |
| `0x24000000` | Selector, then the chosen firmware |
| `0x30000000`–`0x30008000` | DMA-accessible USB/FatFs and LED buffers |
| `0x30008000`–`0x30040000` | Staged original image |
| `0x38000000`–`0x38000400` | Stackless terminal copy/jump routine |

The selector masks interrupts, resets memory-writing DMA/USB/peripherals, disables
runtime/cache state, rechecks staged bytes, and copies/verifies the small jump
routine into separate SRAM4. That routine replaces the selector in AXI RAM,
clears the DMA arena, sets interrupt vectors and stack, then enters the original
reset handler. Vendor binaries and reset vectors are never patched. Existing
bootloader clock/MPU state is retained as required by BOOT_SRAM startup.

## Why USB

The former installed bundle assumed that space outside vendor executables was
unused. We have not established that: vendor firmware may use it for settings or
other data. Smaller binaries and startup passes do not prove ownership. Dirt also
requires the same fixed flash entry as the stock application, forcing reset-word
patching and a resident wrapper. Compression would resolve neither problem.

USB dependence and dropping Dirt remove those layout assumptions and exceptions.
There are no installed vendor slots, wrapper, flash journal or selector flash-write
driver. Our responsibility is preventing added lasting damage, wear or recovery
problems caused by our code and handoff. Equivalent stock vendor behavior is the
baseline. We still need to establish that supported firmware preserves the
selector's application region and that our inherited state causes no harmful
persistent differences. A RAM jump is not a hardware reset or sandbox.

Local and virtual checks do not qualify physical USB, audio, interrupted updates
or stock recovery. See the [release gate](development/recovery_release_gate.md).
