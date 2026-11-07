# AuroraSwitch

Switch RAM firmware without a computer.

**Important**
* You may want to avoid using this for performances until this is better tested.


## Quick start

**Keep USB connected to select firmware and use automatic boot.**

1. Download [AuroraSwitch.bin](https://github.com/RyanJarv/AuroraSwitch/releases/latest/download/AuroraSwitch.bin).
2. Back up a FAT32 USB drive. Put the selector BIN at its root as the only root BIN.
   Follow the stock USB installation procedure.
3. Put exact supported originals in an `aurora/` folder on that drive, using
   their catalog filenames. The selector installer can stay at the root.
4. Keep the drive connected and restart. Select and launch firmware from the menu.

### Select firmware

- **Shift** — next category: blue reverbs, green delays, amber synths/other.
- **Reverse** — next firmware; the arc shows its position and category color.
- **Freeze** — verify, remember and launch.
- **Any button held at power-on** — open the menu.

Your last selection starts automatically with USB connected. Edit
`aurora/choice.txt` to change it, or delete the file to start in the menu.
Missing or invalid firmware leaves the menu open; check USB and press Freeze to retry.

**Dirt Verb replaces the selector; returning requires a computer.** Put
`DirtVerb 1.1.bin` in `aurora/`, select Dirt and press Freeze twice when prompted.
To return, move Dirt back into `aurora/` and rename `AuroraSwitch.disabled` to
`AuroraSwitch.bin`. See the [Dirt steps](docs/user_guide.md#dirt-verb-install-and-restart)
for installation and troubleshooting.

To restore stock firmware, use its original BIN as the only root BIN and follow
[Qu-Bit's normal USB update procedure](https://www.qubitelectronix.com/faq).

## Firmware

The [firmware reference](docs/firmware_reference.md) lists exact supported
versions, download sources, creator credits, colors and controls. Release and
development entries are marked separately. Current source includes Dirt Verb
1.1 through stock USB installation; other flash-running images are unsupported.

Unsupported versions or changed files are rejected. Physical reliability and
stock USB recovery remain open; FataMorgana's USB wavetable path is unverified.

## More Info

- [Online firmware reference](https://aurora-switch.ryanjarv.sh/): color lookup and controls.
- [Firmware control reference](docs/firmware_reference.md): colors and controls for every catalog entry.
- [User guide](docs/user_guide.md): installation, USB layout, and firmware selection.
- [How it works](docs/how_it_works.md): RAM handoff, memory layout, design choices.
- [Development guide](docs/development/README.md): tests and adding firmware.
- [Documentation index](docs/README.md): current guides and validation evidence.
