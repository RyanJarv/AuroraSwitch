# How it works

AuroraSwitch keeps the selector installed and loads supported original firmware
from USB into RAM. The selector never programs or erases on-module flash,
initializes persistent settings there, or changes flash protection. Installing
it through the stock updater necessarily writes its application image.

## Loading and automatic boot

The selector scans catalog filenames and sizes when USB becomes available.
Presence alone does not authorize launch: it reads the chosen file into separate
staging RAM and checks its complete SHA-256, size and startup vectors against the
catalog. Changed, truncated and unsupported files are rejected.

A valid saved choice in `aurora/choice.dat` launches once at startup when USB and
its firmware are available. Hold Shift + Reverse at power-on to open the menu.
Missing or invalid files leave the menu/error state; reconnect USB and press
Freeze to retry. The selector never automatically substitutes another image.

Only changed, authenticated manual choices write settings. Automatic boots and
repeat choices do not. A failed save warns and still permits the verified launch
for that session. An interrupted save may lose the choice, returning to the menu
next boot; it cannot change on-module settings.

## RAM handoff

The selector and RAM firmware run at the same address. To replace itself safely,
the selector authenticates the staged bytes, stops memory-writing peripherals,
disables its runtime and caches, and authenticates those bytes again. A small
routine in separate SRAM copies the image over the selector, clears its DMA
buffers, sets the new interrupt table and stack, and enters the original reset
handler. Vendor binaries and reset vectors are never patched.

This is not a hardware reset or sandbox. Vendor startup initializes its globals;
other RAM can retain earlier contents. The handoff preserves the bootloader's
clock and memory policy required by the supported images. See the
[architecture](development/architecture.md) for memory bounds and startup details.

## Dirt installation

Dirt must run from the stock application flash address. It uses a manual stock
installation action instead of the RAM handoff: verify, confirm with a separate
Freeze press, prepare USB, and restart into the stock updater. Navigation or USB
disconnection cancels confirmation. Dirt is never remembered or auto-installed.

The selector preserves its own installer as `AuroraSwitch.disabled`, writes
return instructions, and copies Dirt to root `DIRTVERB.BIN`. The original stays
in `aurora/`. It checks every resulting file before resetting. After a failed
file-changing operation it stops without reset, rollback or write retries;
inspect the drive on a computer. FAT operations are not power-loss atomic.

The stock updater replaces the selector with Dirt. Returning requires a computer:
[delete the Dirt copy and re-enable the selector](user_guide.md#dirt-verb-install-and-restart).
The selector itself issues no flash commands; the subsequent stock installation
writes flash in the ordinary way.

Local and virtual checks do not qualify physical USB, audio, interrupted updates
or stock recovery. See the [physical gate](development/recovery_release_gate.md).
