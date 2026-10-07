# User guide

Beta software: keep the original Aurora firmware and a backup of your USB drive.

For the single-command release download and USB copy, use the
[README quick start](../README.md#quick-start).

For selector colors and each firmware's knobs, buttons, gates and modes, use the
[firmware control reference](firmware_reference.md).

## Build

Requires Git, GNU Make, Python 3, a C/C++ compiler, and GNU Arm Embedded
**10-2020-q4-major** on PATH.

```sh
git clone https://github.com/RyanJarv/AuroraSwitch.git
cd AuroraSwitch
make check
make package
```

Use `AuroraSwitch.bin` from the printed `dist/<manifest-sha256>/` folder.
Packaging fetches dependencies and requires a clean committed checkout.
For a faster incremental build, use `make -j2 build`; its BIN is at
`firmware/build-experimental-dma/AuroraSwitch.bin`. Never install a virtual build.

## Supported firmware

Obtain these files from their authors; they are not included here.
Only the exact supported versions work. Public download commands are in
[firmware support](alternative_firmware.md#obtain-files).

| Firmware | Filename | Reverse color |
| --- | --- | --- |
| FDN 1.2.2 | `AR_FDN_v1_2_2.bin` | Blue |
| Aurora 1.4.4 | `Aurora_v1_4_4.bin` | Green |
| EchoGarden 0.3.1 | `AuroraEchoGarden_v0_3_1_STABLE.bin` | Cyan |
| CloudscapeX | `AuroraCloudscapeX.bin` | Magenta |
| The Oscillator Is a Lie 0.0.2 | `TheOscillatorIsALie_v0_0_2.bin` | Amber |
| Flux Capacitor 0.3.0 | `flux-capacitor-0.3.0.bin` | Yellow |
| Morse 0.2.0 | `aurora-morse-0.2.0.bin` | White |

Check and prepare whichever supported files you have:

```sh
python3 scripts/prepare_payloads.py --output prepared-payloads \
  local-firmware/AR_FDN_v1_2_2.bin local-firmware/Aurora_v1_4_4.bin
```

Use a new output folder each time. Copy its `aurora/` folder to a compatible FAT
USB drive, alongside the selector BIN:

```text
USB root/
  AuroraSwitch.bin
  aurora/
    AR_FDN_v1_2_2.bin
    Aurora_v1_4_4.bin
```

Keep `AuroraSwitch.bin` as the only BIN at the root; preserve existing settings.
Safely eject, insert into Aurora, and power cycle to install using
[Qu-Bit's normal updater](https://www.qubitelectronix.com/faq).
Do not interrupt the update. To restore stock, replace the root BIN with the
original Aurora BIN and repeat; this still needs testing with AuroraSwitch.

## Select firmware

1. Wait for scanning to finish.
2. Press **Reverse** to choose by color.
3. Press **Freeze** to load it; wait for green.
4. Press **Shift** to start it.

Power cycle to return to the menu. After changing files, reconnect the drive
to rescan. Changing selection or disconnecting requires verification again.
Morse needs an external Freeze-gate clock and Mix above zero.

## Not supported yet

- **Tempest:** QSPI settings writes need a persistence/recovery review.
- **Dirt Verb:** runs from QSPI, outside this RAM loader's design.
- **HP-filter Aurora:** too large for the current staging buffer.
- **Other versions or renamed files:** need separate review and catalog entries.
