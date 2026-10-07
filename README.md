# AuroraSwitch

Choose a supported Aurora firmware from a USB drive and launch it from RAM.
Reverse selects, Freeze verifies, and Shift starts it. Reset or power cycling
returns to the selector through Aurora's existing bootloader.

Beta software with limited testing. Keep the original Aurora firmware and a
backup of your USB drive. More hardware testing is needed.

## Get started

Follow the [user guide](docs/user_guide.md) which covers the basics.

This repo doesn't include the Third-party firmware, you'll need to grab that yourself. Exact supported versions are necessary, currently this project supports:

* [Aurora 1.4.4](https://www.qubitelectronix.com/alternate-firmware/p/aurora-spectral-reverb)
* [FDN 1.2.2](https://www.qubitelectronix.com/alternate-firmware/p/fdn-verb)
* [EchoGarden 0.3.1](https://discord.com/channels/1171549067122311298/1257378715944484864)
* [CloudscapeX](https://discord.com/channels/1171549067122311298/1257378715944484864)
* [The Oscillator Is a Lie 0.0.2](https://discord.com/channels/1171549067122311298/1257378715944484864)
* [Flux Capacitor 0.3.0](https://discord.com/channels/1171549067122311298/1257378715944484864)
* [Morse 0.2.0](https://discord.com/channels/1171549067122311298/1257378715944484864)

Other versions and firmware won't work right now.

## Quick start

Install Git, GNU Make, Python 3, a C/C++ compiler, and GNU Arm Embedded
**10-2020-q4-major**. Then run:

```sh
git clone https://github.com/RyanJarv/AuroraSwitch.git && cd AuroraSwitch
make -j2 build && make check
```

Output: `firmware/build-experimental-dma/AuroraSwitch.bin`.
The build fetches its source dependencies. It does not download payload firmware
or program a device. Follow the [user guide](docs/user_guide.md) to prepare the
USB drive and install it.

Use `make package` for a fresh build with a manifest, ELF, and MAP under
`dist/<manifest-sha256>/`. It requires a clean committed checkout.
Run `make help` for other targets.

## Read more

- [User guide](docs/user_guide.md): build, USB layout, and firmware selection.
- [How it works](docs/how_it_works.md): RAM handoff, memory layout, design choices.
- [Development guide](docs/development/README.md): tests and adding firmware.
- [Documentation index](docs/README.md): current plans and historical evidence.
