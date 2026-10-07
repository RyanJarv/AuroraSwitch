# User guide

Beta software: keep the original Aurora firmware and a backup of your USB drive.

For release downloads and USB setup without a checkout, use the
[README quick start](../README.md#quick-start).

For selector colors and each firmware's knobs, buttons, gates and modes, use the
[firmware control reference](firmware_reference.md).

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

Copy the supported BINs into an `aurora/` folder on a FAT USB drive,
alongside the downloaded selector BIN:

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

Building and optional host-side file verification are covered in the
[development guide](development/README.md). They are not required to install
a published release. Never install a virtual build.

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

## Not supported yet

- **FataMorgana's upstream QSPI build:** unsupported. See the RAM experiment below.
- **Dirt Verb:** runs from QSPI, outside this RAM loader's design.
- **HP-filter Aurora:** too large for the current staging buffer.
- **Other versions or renamed files:** need separate review and catalog entries.

## Development branch additions

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

### FataMorgana RAM experiment

The development branch admits one exact RAM build from source commit `d5504d7`,
not the upstream QSPI configuration. Virtual handoff/control/audio checks pass;
USB wavetable loading and physical compatibility remain unverified.

With the pinned GNU Arm 10 toolchain on PATH:

```sh
make fata-ram-probe
```

The command prints a bundle path. Its `FataMorgana.bin` belongs in `aurora/`
on the test drive; the selector menu color is azure. Use only the development
selector, not the published release. Do not rename another build to this filename.
Tempest and Fata share a settings sector with different formats, so switching
can change retained settings.
