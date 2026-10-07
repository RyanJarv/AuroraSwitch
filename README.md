# AuroraSwitch

Choose a supported Aurora firmware from a USB drive and launch it from RAM.
Reverse selects, Freeze verifies, and Shift starts it. Reset or power cycling
returns to the selector through Aurora's existing bootloader.

Beta software with limited testing. Keep the original Aurora firmware and a
backup of your USB drive. More hardware testing is needed.

## Get started

Follow the [user guide](docs/user_guide.md) which covers the basics.

Firmware is not included. Only these exact versions are supported:

* [Aurora 1.4.4](https://www.qubitelectronix.com/alternate-firmware/p/aurora-spectral-reverb)
* [FDN 1.2.2](https://www.qubitelectronix.com/alternate-firmware/p/fdn-verb)
* [EchoGarden 0.3.1](https://discord.com/channels/1171549067122311298/1257378715944484864)
* [CloudscapeX](https://discord.com/channels/1171549067122311298/1257378715944484864)
* [The Oscillator Is a Lie 0.0.2](https://discord.com/channels/1171549067122311298/1257378715944484864)
* [Flux Capacitor 0.3.0](https://github.com/DaveParr/aurora-flux-capacitor/releases/tag/v0.3.0)
* [Morse 0.2.0](https://github.com/DaveParr/Aurora-Morse/releases/tag/v0.2.0)

Other versions and firmware won't work right now.

## Quick start

Install Git, GNU Make, Python 3, a C/C++ compiler, curl, unzip, and GNU Arm Embedded
**10-2020-q4-major**, with its `bin` directory on PATH. Back up your FAT USB drive
and move any root-level BINs off it first. Change `USB_DIR` below to its mounted
path (Linux example: `/media/your-user/AURORA`). Run from a fresh checkout:

```sh
git clone https://github.com/RyanJarv/AuroraSwitch.git && cd AuroraSwitch
make -j2 build && make check
mkdir -p local-firmware
curl -fL -o local-firmware/Aurora_v1_4_4.zip https://www.qubitelectronix.com/s/Aurora_v1_4_4.zip && unzip local-firmware/Aurora_v1_4_4.zip Aurora_v1_4_4.bin -d local-firmware
curl -fL -o local-firmware/AR_FDN_v1_2_2.bin https://www.qubitelectronix.com/s/AR_FDN_v1_2_2.bin
python3 scripts/prepare_payloads.py --output prepared-payloads local-firmware/Aurora_v1_4_4.bin local-firmware/AR_FDN_v1_2_2.bin
USB_DIR="/Volumes/AURORA"
cp -R prepared-payloads/aurora "$USB_DIR/" && cp firmware/build-experimental-dma/AuroraSwitch.bin "$USB_DIR/" && sync
```

Safely eject the drive, insert it into Aurora, and power cycle to install the
selector. Do not interrupt the update. **Reverse** selects blue (FDN) or green
(original Aurora); **Freeze** loads and verifies it, then **Shift** launches it
once Freeze is green. Power cycle to return to the selector.

The payload check must pass before copying. Settings are left in place;
same-name firmware files are replaced. The downloads come
from [Qu-Bit's official firmware page](https://www.qubitelectronix.com/alternate-firmware/aurora),
not this repository. See the [user guide](docs/user_guide.md) for other supported
images and stock restoration.

Use `make package` for a fresh build with a manifest, ELF, and MAP under
`dist/<manifest-sha256>/`. It requires a clean committed checkout.
Run `make help` for other targets.

## Read more

- [User guide](docs/user_guide.md): build, USB layout, and firmware selection.
- [How it works](docs/how_it_works.md): RAM handoff, memory layout, design choices.
- [Development guide](docs/development/README.md): tests and adding firmware.
- [Documentation index](docs/README.md): current plans and historical evidence.
