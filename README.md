# AuroraSwitch

Choose a supported Aurora firmware from a USB drive and launch it from RAM.
Reverse selects, Freeze verifies, and Shift starts it. Reset or power cycling
returns to the selector through Aurora's existing bootloader.

Beta software with limited testing. Keep the original Aurora firmware and a
backup of your USB drive. More hardware testing is needed.

## Get started

Follow the [user guide](docs/user_guide.md) to build the selector, prepare a drive,
install it, and switch firmware. Third-party firmware is not included; obtain
supported files from their authors.

The catalog accepts seven exact images: Aurora 1.4.4, FDN 1.2.2, EchoGarden
0.3.1, CloudscapeX, The Oscillator Is a Lie 0.0.2, Flux Capacitor 0.3.0, and
Morse 0.2.0. Other versions are not automatically supported.

## Quick start

Install Git, GNU Make, Python 3, a C/C++ compiler, and GNU Arm Embedded
**10-2020-q4-major**. Put the Arm toolchain's `bin` directory on PATH, then run:

```sh
#!/bin/sh
set -eu
git clone https://github.com/RyanJarv/AuroraSwitch.git
cd AuroraSwitch
make -j2 build
make check
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
