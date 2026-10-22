# Installation and firmware selection

**USB is required to launch firmware, including automatic boot.** This development
build still needs physical installation, handoff and stock recovery qualification.

## Prepare and install

1. Build `AuroraSwitch.bin` with `make package` ([build setup](development/README.md)).
2. Back up a FAT32 USB drive. Put that selector BIN at the root as the only root BIN.
3. Safely eject, insert into Aurora, then follow the ordinary stock USB update procedure.
4. After installation, remove `AuroraSwitch.bin` from the root. Create `aurora/`
   and put supported original firmware BINs there with their exact catalog filenames.
5. Keep the drive connected for firmware selection and automatic boot.

Only our selector is installed. Original vendor files are read into RAM unchanged;
no private composite installer or preparer is needed. Firmware-owned tables/options
keep their ordinary USB locations. FataMorgana's USB wavetable path remains unqualified.

## Controls

**Shift — next category. Reverse — next firmware. Freeze — verify & launch.**
Hold **Shift + Reverse at power-on** for the menu. Otherwise the valid saved choice
starts when USB and the exact firmware are available. An absent, corrupt or
unsupported choice opens the menu without substituting another firmware.

Menu colors and firmware controls are in the [reference](firmware_reference.md).
The arc shows position within the category; Reverse shows the firmware color.
Amber Freeze means loading; persistent red means unavailable USB, no usable files
or a failed load. Reconnect USB to rescan, or press Freeze to retry. If a retry
finds files on an empty menu, review the selection and press Freeze again to launch.

The selector saves changed authenticated choices to `aurora/choice.dat`. A brief
red Freeze warning before launch means saving failed; verified firmware can run
for this session. Automatic boot and selecting the saved choice do not rewrite
settings. A damaged choice file can be deleted to return to the menu.

## Supported firmware

Use the exact versions and filenames in the [reference](firmware_reference.md).
Files are checked by complete SHA-256, size and RAM startup vectors. Changed,
truncated, unsupported and flash-running images are rejected. Dirt Verb is not
supported. Adding/removing supported originals on USB changes the available menu.
New versions need review before entering the catalog.

## Stock restoration

Put the original stock BIN at the USB root as the only root BIN and follow
[Qu-Bit's updater instructions](https://www.qubitelectronix.com/faq).
Restoration uses the stock bootloader and must not depend on a working selector
or access to its menu. Power-interruption recovery remains physically unverified.
