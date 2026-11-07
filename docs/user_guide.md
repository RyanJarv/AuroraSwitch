# Installation and firmware selection

**Keep USB connected to select firmware and use automatic boot.**

## Prepare and install

1. Download [AuroraSwitch.bin](https://github.com/RyanJarv/AuroraSwitch/releases/latest/download/AuroraSwitch.bin).
2. Back up a FAT32 USB drive. Put `AuroraSwitch.bin` at its root as the only root BIN.
3. Add supported firmware to `aurora/`, using the exact filenames in the [reference](firmware_reference.md).
4. Safely eject, insert into Aurora and follow [Qu-Bit's USB update procedure](https://www.qubitelectronix.com/faq).
   Leave `AuroraSwitch.bin` at the root. Keep USB connected for selection and automatic boot.

## Controls

**Shift — next category. Reverse — next firmware. Freeze — verify & launch.**

### Startup and status

Your last selection starts automatically with USB connected. Hold **any button at power-on** for the menu.
Missing or invalid files leave the menu open.

**Amber Freeze:** loading. **Red:** check the files or reconnect USB, then press Freeze to retry.
A brief red flash means the choice could not be saved; firmware still launches.

## Saved choice

Select firmware to remember it, or edit `aurora/choice.txt` on a computer.
Use one exact firmware filename, without a folder or quotes:

```text
Aurora_v1_4_4.bin
```

Delete this file to start in the menu. Dirt cannot be saved as an automatic choice.

## Supported firmware

Download the exact versions in the [reference](firmware_reference.md).
Changed or unsupported files are rejected. Third-party firmware is not included.
Keep any firmware-specific tables or options in their usual USB locations.
FataMorgana's USB wavetable support is unverified.

## Dirt Verb: install and restart

**Dirt replaces the selector; returning requires a computer.**

1. Put `DirtVerb 1.1.bin` in `aurora/`. Keep `AuroraSwitch.bin` at the root as the only root BIN.
2. Select Dirt (pink, Reverbs) and press **Freeze**.
3. When Freeze alternates amber/red, release and press **Freeze** again to install.
   **Shift** or **Reverse** cancels.

AuroraSwitch moves Dirt to the root, saves its own installer as `AuroraSwitch.disabled`,
and restarts. **Keep USB and power connected until installation finishes.**
Dirt then runs without USB. Your previous automatic choice stays saved.

### Return to AuroraSwitch

On a computer:

1. Move root `DirtVerb 1.1.bin` back into `aurora/`.
2. Rename `AuroraSwitch.disabled` to `AuroraSwitch.bin`.
3. Safely eject and restart. Hold any button at power-on for the menu.

These steps are also saved on USB in `RETURN TO AURORASWITCH.txt`.
Do not use `AuroraSwitch.bin.disabled`: the updater still treats it as a BIN.

### If switching fails

If Freeze stays red or loading stalls, inspect USB on a computer before restarting.
Keep only the intended installer BIN at the root. Conflicting installer, backup or
return-instruction files block the switch; back them up before removing them.
If automatic restart misses USB, power-cycle with the correct installer at the root.

## Stock restoration

Put the original stock firmware BIN at the root as the only root BIN, then follow
[Qu-Bit's USB update procedure](https://www.qubitelectronix.com/faq).
This works independently of the selector menu.

Physical installation, handoff, software-reset USB discovery and interrupted-update
recovery remain unqualified. Back up your drive before updating.
