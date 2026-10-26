# Installation and firmware selection

**USB is required for RAM firmware, including automatic boot, and to install Dirt.** This development
build still needs physical installation, handoff and stock recovery qualification.

## Prepare and install

1. Build `AuroraSwitch.bin` with `make package` ([build setup](development/README.md)).
2. Back up a FAT32 USB drive. Put that selector BIN at the root as the only root BIN.
3. Safely eject, insert into Aurora, then follow the ordinary stock USB update procedure.
4. After installation, remove `AuroraSwitch.bin` from the root, or rename it
   `AuroraSwitch.img` to keep a restoration copy. Create `aurora/`
   and put supported original firmware BINs there with their exact catalog filenames.
5. Keep the drive connected for firmware selection and automatic boot.

RAM selection keeps our selector installed. Original vendor files are read into RAM unchanged;
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
Files are checked by complete SHA-256, size and their catalog startup vectors.
Changed, truncated and unsupported images are rejected. Dirt is the only stock
USB installation entry; it cannot use the direct RAM launch. Adding/removing supported originals on USB changes the available menu.
New versions need review before entering the catalog.

## Dirt Verb: install and restart

**Dirt replaces the selector; returning requires a computer.**

Put the exact `DirtVerb 1.1.bin` in `aurora/`. Leave no BIN files in the USB root;
`AuroraSwitch.img` is an optional inactive backup. Names such as
`AuroraSwitch.bin.disabled` still count as BIN files and must be removed.

Select Dirt (pink, Reverbs) and press Freeze to verify. When Freeze alternates
amber/red, press Freeze again to confirm, or Shift/Reverse to cancel. The selector
moves Dirt to root `DIRTVERB.BIN`, verifies it there, and restarts. The stock
bootloader installs Dirt, replacing the selector. Do not remove USB or power
while switching/installing. Dirt then boots normally, including without USB.
Your remembered RAM choice is unchanged; the selector never automatically
performs a Dirt installation.

Persistent red after a failed file switch requires computer inspection. Do not
retry or power-cycle with an unverified root BIN. Interrupted FAT operations can
leave damaged directory entries; keep the drive backed up.

To return to AuroraSwitch, use a computer:

1. Move root `DIRTVERB.BIN` back to `aurora/DirtVerb 1.1.bin`.
2. Rename `AuroraSwitch.img` to `AuroraSwitch.bin`, or copy a fresh installer to
   the root. It must be the only root BIN.
3. Restart using the stock USB update procedure. Remove or rename the root
   installer afterward. Your previous valid RAM choice can automatically boot.

Software-reset USB discovery, Dirt installation and interrupted-update recovery
remain physically unqualified. If the automatic restart does not discover USB,
power-cycle with verified media prepared for the intended stock update.

## Stock restoration

Put the original stock BIN at the USB root as the only root BIN and follow
[Qu-Bit's updater instructions](https://www.qubitelectronix.com/faq).
Restoration uses the stock bootloader and must not depend on a working selector
or access to its menu. Power-interruption recovery remains physically unverified.
