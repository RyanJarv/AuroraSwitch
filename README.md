# AuroraSwitch

Choose a supported Aurora firmware from a USB drive and launch it from RAM. Reverse selects, Freeze verifies, and Shift starts it. Reset or power cycling returns to the selector through Aurora's existing bootloader.

Beta software with limited testing. Keep the original Aurora firmware and a backup of your USB drive. More hardware testing is needed.

## Quick start

Back up a FAT USB drive. No repository checkout or build tools are needed.

1. [Download AuroraSwitch.bin](https://github.com/RyanJarv/AuroraSwitch/releases/latest/download/AuroraSwitch.bin)
   to the drive's root. Keep it as the only BIN there.
2. Create an `aurora/` folder. Add [Aurora 1.4.4](https://www.qubitelectronix.com/s/Aurora_v1_4_4.zip)
   (unzip first), [FDN 1.2.2](https://www.qubitelectronix.com/s/AR_FDN_v1_2_2.bin), or both.
3. Safely eject, insert the drive into Aurora, and power cycle to install.

### Select firmware

1. **Reverse** selects by color.
2. **Freeze** loads and verifies; wait for green.
3. **Shift** launches.

Power cycle to return to the selector.

Other supported firmware BINs go in the same `aurora/` folder. Files shared on
Discord must be added manually. See the [user guide](docs/user_guide.md).

## Firmware

### Supported

Only these exact versions are supported:

* [Aurora 1.4.4](https://www.qubitelectronix.com/alternate-firmware/p/aurora-spectral-reverb)
* [FDN 1.2.2](https://www.qubitelectronix.com/alternate-firmware/p/fdn-verb)
* [EchoGarden 0.3.1](https://discord.com/channels/1171549067122311298/1257378715944484864)
* [CloudscapeX](https://discord.com/channels/1171549067122311298/1257378715944484864)
* [The Oscillator Is a Lie 0.0.2](https://discord.com/channels/1171549067122311298/1257378715944484864)
* [Flux Capacitor 0.3.0](https://github.com/DaveParr/aurora-flux-capacitor/releases/tag/v0.3.0)
* [Morse 0.2.0](https://github.com/DaveParr/Aurora-Morse/releases/tag/v0.2.0)

### Not supported yet

- **FataMorgana:** upstream QSPI build is unsupported; a RAM experiment is on the development branch.
- **Dirt Verb:** runs from QSPI, outside this RAM loader's design.
- **HP-filter Aurora:** too large for the current staging buffer.
- **Other versions or renamed files:** need separate review and catalog entries.

### Development branch additions

`codex/older-firmware-onboarding` also supports Flux 0.1.0/0.2.0, Morse 0.1.0,
and Tempest 1.0.0 in virtual launch/control/audio tests. These are not in
the published release yet; physical testing and ordinary stock recovery remain
open. Tempest stores its own settings in QSPI. See the [user guide](docs/user_guide.md#development-branch-additions).

The same branch stages an exact FataMorgana RAM build. Its virtual selector
handoff/control/audio checks pass; USB wavetable loading and physical testing
remain open. It is not part of the published release.

## Video

https://github.com/user-attachments/assets/828c759e-82dc-492c-8cff-8abad066c41e

## More Info

- [Online firmware reference](https://aurora-switch.ryanjarv.sh/): color lookup and controls.
- [Firmware control reference](docs/firmware_reference.md): colors and controls for every catalog entry.
- [User guide](docs/user_guide.md): build, USB layout, and firmware selection.
- [How it works](docs/how_it_works.md): RAM handoff, memory layout, design choices.
- [Development guide](docs/development/README.md): tests and adding firmware.
- [Documentation index](docs/README.md): current plans and historical evidence.
