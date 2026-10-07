# AuroraSwitch

Choose a supported Aurora firmware from a USB drive and launch it from RAM.
Reverse selects, Freeze verifies, and Shift starts it. Reset or power cycling
returns to the selector through Aurora's existing bootloader.

Beta software with limited testing. Keep the original Aurora firmware available.
Physical reliability and ordinary stock USB recovery still need testing.

## Get started

Follow the [user guide](docs/user_guide.md) to build the selector, prepare a drive,
install it, and switch firmware. Third-party firmware is not included; obtain
supported files from their authors.

The catalog accepts seven exact images: Aurora 1.4.4, FDN 1.2.2, EchoGarden
0.3.1, CloudscapeX, The Oscillator Is a Lie 0.0.2, Flux Capacitor 0.3.0, and
Morse 0.2.0. Other versions are not automatically supported.

## Build

Requires Git, GNU Make, Python 3, a host C/C++ compiler, and GNU Arm Embedded
**10-2020-q4-major** on PATH.

```sh
git clone https://github.com/RyanJarv/AuroraSwitch.git
cd AuroraSwitch
make dependencies -j2
make -j2
make test
```

The USB build is `firmware/build-experimental-dma/AuroraSwitch.bin`.
For an isolated build with recorded source, toolchain, and artifact identities,
run `make package` from a clean committed checkout. Output goes to
`dist/<manifest-sha256>/`. No command here programs a device.

## Read more

- [User guide](docs/user_guide.md): installation, controls, recovery, troubleshooting.
- [How it works](docs/how_it_works.md): RAM handoff, memory layout, design choices.
- [Development guide](docs/development.md): tests and adding firmware.
- [Verification status](docs/verification.md): what is checked and what remains open.
- [Documentation index](docs/README.md): current plans and historical evidence.
