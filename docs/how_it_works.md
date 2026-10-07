# How it works

AuroraSwitch is an application, not a replacement bootloader. It loads a
supported firmware file from USB into RAM and starts it.

## Why this approach?

Aurora's existing bootloader already starts RAM-based firmware. Reusing it
keeps the normal updater in place and avoids flashing a payload each time
you switch. A small catalog limits loading to reviewed files.

The tradeoff is startup state: jumping to another program is not a hardware
reset. The selector must stop its peripherals and clear shared DMA buffers
before the next firmware starts. Switching interrupts audio; it is not seamless.

## What happens

1. Aurora's bootloader starts the installed selector.
2. The selector checks known files on the drive and shows supported matches.
3. Reverse chooses a file; Freeze loads and verifies its complete contents.
4. Shift verifies those same RAM bytes again, stops USB, and cleans up interrupts
   and peripherals.
5. A small routine running elsewhere in RAM copies the firmware over the
   selector, clears the DMA arena, and jumps to its startup code.

The selector is no longer running afterward. The selected firmware owns the
module; reset returns through the existing bootloader.

Loading a payload does not write flash. Installing the selector does, and
selected firmware may save its own settings. File hashes identify expected
bytes, not bug-free software. Physical reliability and stock restore still
need testing.

Memory regions and cleanup details are in the
[developer architecture note](development/architecture.md).
