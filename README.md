# AuroraSwitch

Allows you to select which Aurora firmware you want to load when starting the module. This does not replace the official boot loader, to uninstall just change the file on the SD Card back to whatever you used before.

**Important**
* *Constantly booting non-ram builds of firmware like Dirt Verb or changing the currently selected firmware can wear out flash faster than normal. I'd avoid using Dirt Verb or check back for an update if that's a concern. For context heavy use here would be loading Dirt Verb or changing the selected firmware 10+ times a day for 10+ years. Repeatedly booting firmware besides Dirt Verb (or other non-RAM builds) don't have this issue.*
* You may to avoid using this for performances until this is better tested.


## Quick start

Back up a FAT USB drive. No repository checkout or build tools are needed.

1. [Download AuroraSwitch.bin](https://github.com/RyanJarv/AuroraSwitch/releases/latest/download/AuroraSwitch.bin)
   to the drive's root. Keep it as the only BIN there.
2. Create an `aurora/` folder. Add [Aurora 1.4.4](https://www.qubitelectronix.com/s/Aurora_v1_4_4.zip)
   (unzip first), [FDN 1.2.2](https://www.qubitelectronix.com/s/AR_FDN_v1_2_2.bin), or both.
3. Safely eject, insert the drive into Aurora, and power cycle to install.

### Select firmware

For the current source build:

1. **Shift** changes category: blue reverbs, green delays, amber synths/other.
2. **Reverse** selects firmware, keeping its original color. One arc shows its position and category color.
3. **Freeze** verifies and launches. Red means loading failed; choose or retry.

The older published release uses Reverse → Freeze → Shift instead.

Power cycle to return to the selector.

Other supported firmware BINs go in the same `aurora/` folder. Files shared on
Discord must be added manually. See the [user guide](docs/user_guide.md).

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
[user guide](docs/user_guide.md#source-build-additions) for files and commands.

Earlier exact-build virtual screens pass; physical reliability and stock USB
recovery remain open. Fata's USB wavetable path is unverified. Unsupported
versions or changed files are rejected.

## More Info

- [Online firmware reference](https://aurora-switch.ryanjarv.sh/): color lookup and controls.
- [Firmware control reference](docs/firmware_reference.md): colors and controls for every catalog entry.
- [User guide](docs/user_guide.md): build, USB layout, and firmware selection.
- [How it works](docs/how_it_works.md): RAM handoff, memory layout, design choices.
- [Development guide](docs/development/README.md): tests and adding firmware.
- [Documentation index](docs/README.md): current plans and historical evidence.
