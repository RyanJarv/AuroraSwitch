# Installation and firmware selection

**USB is required for RAM firmware, including automatic boot, and to install Dirt.** This development
build still needs physical installation, handoff and stock recovery qualification.

## Prepare and install

1. Build `AuroraSwitch.bin` with `make package` ([build setup](development/README.md)).
2. Back up a FAT32 USB drive. Put that selector BIN at the root as the only root BIN.
3. Safely eject, insert into Aurora, then follow the ordinary stock USB update procedure.
4. Leave `AuroraSwitch.bin` at the root. Create `aurora/`
   and put supported original firmware BINs there with their exact catalog filenames.
5. Keep the drive connected for firmware selection and automatic boot.

## Controls

**Shift — next category. Reverse — next firmware. Freeze — verify & launch.**

### Startup and status

Hold **any button at power-on** for the menu. Keep USB connected: your last
RAM firmware choice starts automatically when its file is available. Missing or
invalid files leave the menu open.

Amber Freeze means loading. Red means loading failed; reconnect USB or check the
firmware files, then press Freeze to retry.

## Saved choice

`aurora/choice.txt` contains one exact supported RAM firmware filename, for example:

```text
Aurora_v1_4_4.bin
```

Edit that line on a computer to choose the next automatic boot. Use the filename
from the catalog, without a folder or quotes. LF, CRLF or no final newline are
accepted. Unknown names, extra lines and Dirt leave the menu open. The selected
firmware is still fully authenticated before launch.

Manual selections update the file only when the authenticated choice changes.
Automatic boots and repeats do not rewrite it, including user-edited line endings.
A brief red warning means saving failed; verified firmware can run for this session.
Delete the file to open the menu. The old binary `choice.dat` is ignored; select
once or create `choice.txt` to set the choice in the new format.

## Supported firmware

Use the exact versions and filenames in the [reference](firmware_reference.md).
Files are checked by complete SHA-256, size and their catalog startup vectors.
Changed, truncated and unsupported images are rejected. Dirt is the only stock
USB installation entry; it cannot use the direct RAM launch. Adding/removing supported originals on USB changes the available menu.
New versions need review before entering the catalog.

Firmware-owned tables/options keep their ordinary USB locations. FataMorgana's
USB wavetable path remains unqualified.

### Download public releases

For supported Flux Capacitor and Morse releases:

```sh
mkdir -p local-firmware
gh release download v0.3.0 --repo DaveParr/aurora-flux-capacitor \
  --pattern flux-capacitor-0.3.0.bin --dir local-firmware
gh release download v0.2.0 --repo DaveParr/Aurora-Morse \
  --pattern aurora-morse-0.2.0.bin --dir local-firmware
```

Obtain other originals from their authors using the reference links. Discord
files must be supplied manually; no third-party firmware is included here.

## Dirt Verb: install and restart

**Dirt replaces the selector; returning requires a computer.**

Put the exact `DirtVerb 1.1.bin` in `aurora/`. The root `AuroraSwitch.bin` may
stay: the selector verifies it and renames it to `AuroraSwitch.disabled` when
switching. If the installer was removed, the selector saves its installed image
under that name instead. Other root BINs and conflicting backup/instruction
files block the switch. Dirt must not be hidden. Names such as
`AuroraSwitch.bin.disabled` still count as active BIN files.

Select Dirt (pink, Reverbs) and press Freeze to verify. When Freeze alternates
amber/red, press Freeze again to confirm, or Shift/Reverse to cancel. The selector
preserves its own installer, writes `RETURN TO AURORASWITCH.txt`, copies Dirt to
root `DIRTVERB.BIN`, verifies it there, and restarts. Dirt's original stays in `aurora/`. The stock
bootloader installs Dirt, replacing the selector. Do not remove USB or power
while switching/installing. Dirt then boots normally, including without USB.
Your remembered RAM choice is unchanged; the selector never automatically
performs a Dirt installation.

Persistent red after a failed file switch requires computer inspection. Do not
retry or power-cycle with an unverified root BIN. Interrupted FAT operations can
leave damaged directory entries; keep the drive backed up.

To return to AuroraSwitch, use a computer:

1. Delete root `DIRTVERB.BIN`.
2. Rename `AuroraSwitch.disabled` to `AuroraSwitch.bin`.
3. Safely eject and restart using the stock USB update procedure.

These steps are also in `RETURN TO AURORASWITCH.txt` on the drive. Keep only one
active root BIN. Your previous RAM choice may start automatically after restoration;
hold any button at power-on to open the menu.

Software-reset USB discovery, Dirt installation and interrupted-update recovery
remain physically unqualified. If the automatic restart does not discover USB,
power-cycle with verified media prepared for the intended stock update.

## Stock restoration

Put the original stock BIN at the USB root as the only root BIN and follow
[Qu-Bit's updater instructions](https://www.qubitelectronix.com/faq).
Restoration uses the stock bootloader and must not depend on a working selector
or access to its menu. Power-interruption recovery remains physically unverified.
