# AuroraSwitch

Select Aurora firmware from an installed flash bundle. The official bootloader
continues to handle USB installation and stock restoration.

**Development branch:** this single-file installer workflow is not yet a
physically qualified release. Published older releases use the previous USB
firmware-folder workflow.

## Quick start

Development workflow; older published releases use the previous USB layout.

1. Build the development preparer with `make preparer` ([build setup](docs/development/README.md)).
   Open the generated `dist/*-preparer.html` locally. No prebuilt preparer has
   been published yet; older releases use the [v0.1.0 USB workflow](https://github.com/RyanJarv/AuroraSwitch/blob/v0.1.0/README.md).
2. Obtain original firmware from its authors. Add **Dirt Verb 1.1** and any
   other supported versions, such as [Aurora 1.4.4](https://www.qubitelectronix.com/s/Aurora_v1_4_4.zip)
   (unzip first) or [FDN 1.2.2](https://www.qubitelectronix.com/s/AR_FDN_v1_2_2.bin),
   to the preparer, then download **AuroraSwitch.bin**.
   Original files stay unchanged and are not uploaded. Keep the generated BIN private.
3. Back up a FAT USB drive. Put **AuroraSwitch.bin** at its root as the only BIN.
   Safely eject, insert into Aurora, and power on to install.
4. Remove the drive for normal selection and boot. Firmware-specific USB assets,
   such as FataMorgana wavetables or Aurora options, still require a drive.

### Select firmware

- **Shift** — next category: blue reverbs, green delays, amber synths/other.
- **Reverse** — next firmware; the arc shows its position and category color.
- **Freeze** — verify, remember and launch. A brief red warning means saving failed;
  the firmware can still run for this session.
- **Shift + Reverse held at power-on** — open the menu.

The last valid saved choice starts on the next boot without USB. Automatic boots and
reselecting the saved firmware do not write the selection record. Update a bundle
by preparing a new BIN; adding files to a drive does not change the installed menu.

To restore stock firmware, use its original BIN as the only root BIN and follow
[Qu-Bit's normal USB update procedure](https://www.qubitelectronix.com/faq).

## Firmware

### Supported

The published release supports these exact versions:

* [Aurora 1.4.4](https://www.qubitelectronix.com/alternate-firmware/p/aurora-spectral-reverb)
* [FDN 1.2.2](https://www.qubitelectronix.com/alternate-firmware/p/fdn-verb)
* [EchoGarden 0.3.1](https://discord.com/channels/1171549067122311298/1257378715944484864)
* [CloudscapeX](https://discord.com/channels/1171549067122311298/1257378715944484864)
* [The Oscillator Is a Lie 0.0.2](https://discord.com/channels/1171549067122311298/1257378715944484864)
* [Flux Capacitor 0.3.0](https://github.com/DaveParr/aurora-flux-capacitor/releases/tag/v0.3.0)
* [Morse 0.2.0](https://github.com/DaveParr/Aurora-Morse/releases/tag/v0.2.0)

### Source-build additions

Current source adds Flux 0.1.0/0.2.0, Morse 0.1.0, Tempest 1.0.0, and one
FataMorgana RAM build. This branch also includes Dirt Verb 1.1 (QSPI) and
HP-filter Aurora (RAM), with no extra build options. See the
[user guide](docs/user_guide.md#supported-firmware) for files and commands.

Earlier exact-build virtual screens pass; physical reliability and stock USB
recovery remain open. Fata's USB wavetable path is unverified. Unsupported
versions or changed files are rejected.

## Video

https://github.com/user-attachments/assets/828c759e-82dc-492c-8cff-8abad066c41e

## More Info

- [Online firmware reference](https://aurora-switch.ryanjarv.sh/): color lookup and controls.
- [Firmware control reference](docs/firmware_reference.md): colors and controls for every catalog entry.
- [User guide](docs/user_guide.md): installation, USB layout, and firmware selection.
- [How it works](docs/how_it_works.md): RAM handoff, memory layout, design choices.
- [Development guide](docs/development/README.md): tests and adding firmware.
- [Documentation index](docs/README.md): current guides and validation evidence.
