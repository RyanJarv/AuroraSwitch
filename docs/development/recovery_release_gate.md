# Developer note: recovery

Status: **not yet verified**. Keep public releases as drafts until this is tested.

Do not leave the module permanently broken. Supported use must remain recoverable
by installing original Aurora through the normal USB updater.

This gate covers additional risks introduced by AuroraSwitch. Compare
vendor-owned writes with equivalent stock operation; normal vendor behavior is
not something the selector must prevent. See the
[failure-mode scope](failure_modes.md#scope-risks-introduced-by-auroraswitch).

Before release:

- Record the existing bootloader identity, boot/protection configuration,
  calibration and relevant settings before changing hardware state. Keep backups
  and readbacks private. Compare again after installation, selector operation
  and restoration, attributing expected vendor writes against stock behavior.
- Install the complete private bundle through the ordinary USB updater.
- Boot without a drive, including the last saved Dirt and RAM choices.
- Hold Shift + Reverse at power-on, change a selection, then power cycle.
- Interrupt selection-journal writes and confirm the old or new complete choice survives.
- Interrupt bundle installation during representative erase/program phases,
  then restore stock through the ordinary USB updater. Record which phases were
  actually exercised; manual power cuts do not prove every interruption timing.
- Verify USB restoration when the selector/application cannot start. Enter the
  stock loader through its hardware recovery path; recovery must not depend on
  reaching the selector menu or executing application code.
- Test both official launch directions, reset/re-entry, and corrupt bundled images.
- Restore stock through USB and check controls, audio, and calibrated behavior.
- Record the exact builds, hardware, and results. Check for unexplained changes
  to the bootloader, calibration, or other persistent state.

If recovery needs ST-Link, ROM DFU, opening the module, recalibration, or internal
backups, stop and fix the cause or remove the affected support. Software tests
alone cannot establish physical recovery. Do not promise it is guaranteed.
