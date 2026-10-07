# User guide

AuroraSwitch is for early testers with a Qu-Bit Aurora on Daisy Seed.
It launches supported firmware from RAM; switching is a restart, not a seamless
effect change. Physical reliability and ordinary stock recovery remain open.
Keep a known-good original Aurora BIN and a backup of your USB drive.

## Build the selector

Use Git, GNU Make, Python 3, a host C/C++ compiler, and GNU Arm Embedded
**10-2020-q4-major**. Add that toolchain's `bin` directory to PATH.

```sh
git clone https://github.com/RyanJarv/AuroraSwitch.git
cd AuroraSwitch
make dependencies -j2
make -j2
make test
make package
```

Dependency setup fetches pinned source, not firmware or system packages.
Packaging requires a clean committed checkout, rebuilds the application and
libDaisy in isolation, and prints a bundle path under `dist/<manifest-sha256>/`.
Use its `AuroraSwitch.bin`; retain the manifest, ELF, and MAP for test reports.
Do not install an `AuroraSwitchVirtual.bin` or a package made with `--virtual`.

## Prepare the firmware files

Obtain the exact supported files yourself. The table below lists accepted names;
[firmware support](alternative_firmware.md) has public download commands.
Keep your copies in ignored `local-firmware/`.

```sh
python3 scripts/prepare_payloads.py --output prepared-payloads \
  local-firmware/AR_FDN_v1_2_2.bin local-firmware/Aurora_v1_4_4.bin
```

Add any supported files to that command, or use just one. The helper checks the
selector's catalog, copies verified bytes into `prepared-payloads/aurora/`, and
writes `payloads.json`. It refuses changed/unknown images and existing output
directories. Choose a new output directory for another run.

| Firmware | Filename | Reverse color |
| --- | --- | --- |
| FDN 1.2.2 | `AR_FDN_v1_2_2.bin` | Blue |
| Aurora 1.4.4 | `Aurora_v1_4_4.bin` | Green |
| EchoGarden 0.3.1 | `AuroraEchoGarden_v0_3_1_STABLE.bin` | Cyan |
| CloudscapeX | `AuroraCloudscapeX.bin` | Magenta |
| The Oscillator Is a Lie 0.0.2 | `TheOscillatorIsALie_v0_0_2.bin` | Amber |
| Flux Capacitor 0.3.0 | `flux-capacitor-0.3.0.bin` | Yellow |
| Morse 0.2.0 | `aurora-morse-0.2.0.bin` | White |

## Install on Aurora

Use Aurora's compatible FAT USB drive. Back it up; preserve settings and recovery
files. Put the packaged `AuroraSwitch.bin` at the root as the **only root BIN**.
Move any previous root updater BIN to your computer. Copy the prepared `aurora/`
folder alongside it; selected firmware files must stay in that subdirectory.

Example with two payloads:

```text
USB root/
  AuroraSwitch.bin
  aurora/
    AR_FDN_v1_2_2.bin
    Aurora_v1_4_4.bin
```

Safely eject the drive, insert it into Aurora, and power cycle. Qu-Bit's
[normal USB update procedure](https://www.qubitelectronix.com/faq) describes
automatic installation at power-up, indicated by white LEDs. Do not interrupt
the update. This project's exact-build physical campaign is still pending;
the procedure is not a claimed recovery pass for AuroraSwitch.

## Choose and launch

1. Wait for scanning to finish. Only exact supported files enter the menu.
2. Press/release **Reverse** to select a firmware by color.
3. Press/release **Freeze** to load and verify it. Wait for Freeze to turn green.
4. Press/release **Shift** to launch. The selected firmware now owns all controls.

To choose another firmware, power cycle with the selector drive inserted and
repeat. There is no universal return-to-menu button combination in the payloads.
Changing selection or disconnecting the drive revokes approval; verify again.
After editing files on your computer, safely eject and reconnect to rescan.
Morse needs an external Freeze-gate clock and Mix above zero.

## Status and troubleshooting

| Freeze color | Meaning / action |
| --- | --- |
| Dim blue | No ready media; check the drive |
| White | Selection available; press Freeze to verify |
| Amber | Scanning/loading; wait |
| Green | Verified; press Shift to launch |
| Red | No supported image, failed verification, or initialization error |

If a file is missing from the menu, check its version and filename with the
preparation helper. Renaming unsupported bytes does not make them compatible.
Initialization errors stay red until reset. A bad drive can stall upstream
USB/FatFs calls; there is no overall load timeout.

Test controls as well as audio. Sound with frozen controls is not success.
Start monitoring quietly; handoff transients are not characterized. Keep
ST-Link disconnected during current testing because attachment has caused resets.

## Restore original firmware

Back up the drive. Replace the root selector BIN with your known-good original
Aurora BIN, leaving only that updater BIN at the root. Payloads may remain in
`aurora/`. Safely eject, insert into Aurora, and power cycle using the normal
update procedure linked above. Check controls and audio afterward.

Ordinary USB restoration is a [release requirement](recovery_release_gate.md),
not an established always-recoverable guarantee. If the updater cannot restore
normal operation, stop and report the exact build and symptoms. Do not guess
at bootloader, calibration, or debug-programming fixes.
