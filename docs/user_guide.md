# Installation and firmware selection

Current development builds install one private bundle. Existing published
releases predate this workflow. Physical installation and stock recovery of the
new exact build remain qualification steps.

## Prepare and install

1. Generate the development preparer with `make preparer` ([build setup](development/README.md)),
   then open `dist/*-preparer.html` in a desktop browser. A prebuilt preparer is
   not yet published.
2. Select your original Dirt Verb 1.1 download and any other supported firmware
   files obtained from their authors. Files are checked by complete SHA-256,
   size and startup vectors. Unsupported or modified versions are rejected.
3. Download `AuroraSwitch.bin`. Keep it private: it contains third-party firmware.
4. Back up a FAT USB drive. Put that BIN at the root, keeping it the only root BIN.
5. Safely eject, insert into Aurora and power on. The original updater installs
   the bundle. Remove the drive for ordinary selector use.

No original vendor file is modified. The browser assembles prebuilt code locally;
no firmware upload is performed. Once generated, the HTML works offline without
a checkout or build tools.

## Controls

**Shift — next category. Reverse — next firmware. Freeze — verify & launch.**
Hold **Shift + Reverse at power-on** for the menu. Otherwise the last remembered
available firmware starts automatically. No saved choice opens the menu.

Menu colors and firmware controls are in the [reference](firmware_reference.md).
A brief red Freeze LED before launch means saving failed. The firmware can run
for this session, but the next boot may use a previous choice or open the menu.
A red LED remaining in the menu means launch failed.

## Supported firmware

Obtain exact versions from the authors' links in the [reference](firmware_reference.md).
Dirt Verb is required because the resident startup arrangement preserves its
fixed flash execution address. Other catalog images are optional. To add or
remove images, prepare and install another bundle. An absent saved image leaves
the menu open; it does not automatically substitute a different firmware.

## USB assets and restoration

The selector does not mount USB. Firmware that loads its own tables or options
still needs those files on a drive; see its reference page. FataMorgana's USB
wavetable path remains unqualified.

To restore an official firmware, put its original BIN at the USB root as the
only BIN and follow [Qu-Bit's updater instructions](https://www.qubitelectronix.com/faq).
Remove the combined installer from that drive before stock restoration.

## Build the preparer

Developers can run `make check` and `make preparer` with the pinned toolchain.
This exports a self-contained HTML file into `dist/`, containing only our
selector/wrapper code and supported metadata. That HTML can be distributed;
the private BIN assembled from a user's firmware cannot be included in releases.

## Unsupported firmware

Only exact catalog versions are admitted. A different filename does not grant
support. New versions need review of linked execution addresses, memory usage,
startup and persistent writes; see [adding firmware](development/README.md#add-firmware).
For the published v0.1.0 selector, use its
[tagged USB-folder instructions](https://github.com/RyanJarv/AuroraSwitch/blob/v0.1.0/README.md).
