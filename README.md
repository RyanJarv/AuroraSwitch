# AuroraSwitch

Switch firmware without a computer.

**Important**
* You may want to avoid using this for performances until this is better tested.


## Quick start

**USB is required to launch firmware, including automatic boot.**

1. Build `AuroraSwitch.bin` with `make package` ([build setup](docs/development/README.md)).
2. Back up a FAT32 USB drive. Put the selector BIN at its root as the only root BIN.
   Follow the stock USB installation procedure.
3. Remove the root installer after installation. Put exact supported originals
   in an `aurora/` folder on that drive, using their catalog filenames.
4. Keep the drive connected and restart. Select and launch firmware from the menu.

### Select firmware

- **Shift** — next category: blue reverbs, green delays, amber synths/other.
- **Reverse** — next firmware; the arc shows its position and category color.
- **Freeze** — verify, remember and launch. A brief red warning means saving failed;
  the firmware can still run for this session.
- **Shift + Reverse held at power-on** — open the menu.

The last valid saved choice starts on the next boot when USB and its firmware
are available. Choices live in `aurora/choice.dat`. Automatic boots and repeated
choices do not rewrite that file. Missing or invalid files leave the menu open;
reconnect the drive and press Freeze to retry.

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
FataMorgana RAM build. This branch also includes
HP-filter Aurora (RAM), with no extra build options. Dirt Verb and other firmware
requiring execution from application flash are unsupported. See the
[user guide](docs/user_guide.md#supported-firmware) for files and commands.

Earlier exact-build virtual screens pass; physical reliability and stock USB
recovery remain open. Fata's USB wavetable path is unverified. Unsupported
versions or changed files are rejected.

## More Info

- [Online firmware reference](https://aurora-switch.ryanjarv.sh/): color lookup and controls.
- [Firmware control reference](docs/firmware_reference.md): colors and controls for every catalog entry.
- [User guide](docs/user_guide.md): installation, USB layout, and firmware selection.
- [How it works](docs/how_it_works.md): RAM handoff, memory layout, design choices.
- [Development guide](docs/development/README.md): tests and adding firmware.
- [Documentation index](docs/README.md): current guides and validation evidence.
