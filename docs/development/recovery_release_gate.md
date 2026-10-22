# Developer note: recovery

Status: **not yet verified**. Keep public releases as drafts until this is tested.

Do not leave the module permanently broken. Supported use must remain recoverable
by installing original Aurora through the normal USB updater.

Before release:

- Install the complete private bundle through the ordinary USB updater.
- Boot without a drive, including the last saved Dirt and RAM choices.
- Hold Shift + Reverse at power-on, change a selection, then power cycle.
- Interrupt selection-journal writes and confirm the old or new complete choice survives.
- Test both official launch directions, reset/re-entry, and corrupt bundled images.
- Restore stock through USB and check controls, audio, and calibrated behavior.
- Record the exact builds, hardware, and results. Check for unexplained changes
  to the bootloader, calibration, or other persistent state.

If recovery needs ST-Link, ROM DFU, opening the module, recalibration, or internal
backups, stop and fix the cause or remove the affected support. Software tests
alone cannot establish physical recovery. Do not promise it is guaranteed.
