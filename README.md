# AuroraSwitch

Switch RAM firmware without a computer.

**Important**
* You may want to avoid using this for performances until this is better tested.


## Quick start

**USB is required for RAM firmware, including automatic boot, and to install Dirt.**

1. Build `AuroraSwitch.bin` with `make package` ([build setup](docs/development/README.md)).
2. Back up a FAT32 USB drive. Put the selector BIN at its root as the only root BIN.
   Follow the stock USB installation procedure.
3. Put exact supported originals in an `aurora/` folder on that drive, using
   their catalog filenames. The selector installer can stay at the root.
4. Keep the drive connected and restart. Select and launch firmware from the menu.

### Select firmware

- **Shift** — next category: blue reverbs, green delays, amber synths/other.
- **Reverse** — next firmware; the arc shows its position and category color.
- **Freeze** — verify, remember and launch. A brief red warning means saving failed;
  the firmware can still run for this session.
- **Any button held at power-on** — open the menu.

The last valid saved choice starts on the next boot when USB and its firmware
are available. Edit `aurora/choice.txt` with an exact catalog filename to change
the automatic boot choice. Automatic boots and repeated
choices do not rewrite that file. Missing or invalid files leave the menu open;
reconnect the drive and press Freeze to retry.

**Dirt Verb replaces the selector; returning requires a computer.** Select Dirt
and press Freeze to verify. Alternating amber/red Freeze asks for a second press
to copy Dirt to the USB root and restart into the stock updater. Shift or Reverse
cancels. The switch preserves the selector as `AuroraSwitch.disabled`, leaves
Dirt's original in `aurora/`, and writes `RETURN TO AURORASWITCH.txt` at the root.
Other root BINs or conflicting backup/instruction files block the switch.
To return on a computer: delete `DIRTVERB.BIN`, then rename `AuroraSwitch.disabled`
to `AuroraSwitch.bin`. Safely eject and restart. Dirt is never an automatic
selector choice; your previous RAM choice stays remembered.

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
