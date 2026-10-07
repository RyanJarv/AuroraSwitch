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

Colors are fixed per firmware family across versions and installs. Keep one
version per family on the drive: multiple versions have the same menu color.

## Source-build additions

<!-- Keep the old guide link usable. -->
<a id="development-branch-additions"></a>

The current source includes these exact images. Prior virtual
launch/control/audio checks pass; physical testing remains open. Build from
source; the published release does not include these entries yet.
Download the named BIN from each linked release and place it in `aurora/`.

| Firmware | Filename | Reverse color |
| --- | --- | --- |
| [Flux 0.1.0](https://github.com/DaveParr/aurora-flux-capacitor/releases/tag/v0.1.0) | `flux-capacitor-0.1.0.bin` | Yellow |
| [Flux 0.2.0](https://github.com/DaveParr/aurora-flux-capacitor/releases/tag/v0.2.0) | `flux-capacitor-0.2.0.bin` | Yellow |
| [Morse 0.1.0](https://github.com/DaveParr/Aurora-Morse/releases/tag/v0.1.0) | `aurora-morse-0.1.0.bin` | White |
| [Tempest 1.0.0](https://github.com/jfriess/Aurora-Firmwares/releases/tag/Tempest-v1.0.0) | `Tempest_v1_0_0.bin` | Pale red |

Tempest saves its settings in QSPI. This is payload behavior, not selector
flashing; do not assume its settings are isolated from other alternative apps.

### FataMorgana

Current source admits one exact RAM build from source commit `d5504d7`,
not the upstream QSPI configuration. Virtual handoff/control/audio checks pass;
USB wavetable loading and physical compatibility remain unverified.
RAM is our preferred build: it avoids programming application flash on launch.
The duplicate QSPI build is no longer offered in the selector.

With the pinned GNU Arm 10 toolchain on PATH:

```sh
make fata-ram-probe
```

The command prints a bundle path. Its `FataMorgana.bin` belongs in `aurora/`
on the test drive; the selector menu color is azure. Use only the development
selector, not the published release. Do not rename another build to this filename.
Tempest and Fata share a settings sector with different formats, so switching
can change retained settings.

### QSPI development branch

Every build of `codex/qspi-dirt-verb` also supports these exact files:

| Firmware | Filename | Reverse color |
| --- | --- | --- |
| Dirt Verb 1.1 | `DirtVerb 1.1.bin` | Red-pink |
| HP-filter Aurora | `Aurora_v1-4-6_hpfilt.bin` | Lime |

Earlier exact-build virtual screening passes; physical tests remain open.
Only Dirt uses QSPI; HP-filter still launches from RAM. QSPI launching replaces
the installed application. Returning to the selector depends on ready USB media
containing its root BIN. Do not treat reset alone
as guaranteed menu re-entry or recovery.

With the pinned GNU Arm 10 toolchain on PATH:

```sh
make package
```

Use `AuroraSwitch.bin` from the printed sealed bundle. For incremental build
and USB copy, use `make usb USB_DIR="/path/to/drive"`.
Authenticate and name user-supplied Dirt/HP binaries without accessing a drive:

```sh
python3 scripts/prepare_payloads.py --output prepared-payloads /path/to/DirtVerb.bin /path/to/Aurora_v1-4-6_hpfilt.bin
```

Use the emitted filenames under `prepared-payloads/aurora/` and keep the selector
as the only root BIN. No alternative firmware is included in our releases.
Hardware testing and ordinary stock recovery are still required before release.

## Unsupported firmware

Only catalog versions with matching bytes and filenames are accepted. Other
images need memory-layout review and a new catalog entry; renaming is not enough.
The duplicate QSPI FataMorgana option is retired in favor of RAM.
