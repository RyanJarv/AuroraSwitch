# AuroraSwitch

Choose a supported Aurora firmware from a USB drive and launch it from RAM. Reverse selects, Freeze verifies, and Shift starts it. Reset or power cycling returns to the selector through Aurora's existing bootloader.

Beta software with limited testing. Keep the original Aurora firmware and a backup of your USB drive. More hardware testing is needed.

## Quick start

Install Git, GitHub CLI (`gh`), GNU Make, Python 3, and C/C++ compilers.

Back up your FAT USB drive. Change `USB_DIR` below to its mounted path:

```sh
git clone https://github.com/RyanJarv/AuroraSwitch.git && cd AuroraSwitch
make download-release USB_DIR="/Volumes/AURORA"
```

Eject, return USB, power on Aurora.

* **Reverse** selects by color (blue for FDN, green for original Aurora);
* **Freeze** loads and verifies it
* **Shift** launches it once Freeze is green.

Power cycle to return to the selector.

Firmware found on the QuBit Discord must be added manually. See the [user guide](docs/user_guide.md) for more info.

This downloads the latest published selector release and four supported public firmwares. To build from source instead, install GNU Arm Embedded **10-2020-q4-major** on PATH and use `make -j2 usb USB_DIR="/Volumes/AURORA"`.

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

- **Tempest:** QSPI settings writes need a persistence/recovery review.
- **Dirt Verb:** runs from QSPI, outside this RAM loader's design.
- **HP-filter Aurora:** too large for the current staging buffer.
- **Other versions or renamed files:** need separate review and catalog entries.

## Video

https://github.com/user-attachments/assets/828c759e-82dc-492c-8cff-8abad066c41e

## More Info

- [Online firmware reference](https://RyanJarv.github.io/AuroraSwitch/): color lookup and controls.
- [Firmware control reference](docs/firmware_reference.md): colors and controls for every catalog entry.
- [User guide](docs/user_guide.md): build, USB layout, and firmware selection.
- [How it works](docs/how_it_works.md): RAM handoff, memory layout, design choices.
- [Development guide](docs/development/README.md): tests and adding firmware.
- [Documentation index](docs/README.md): current plans and historical evidence.
