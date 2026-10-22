# Physical recovery release gate

**Unverified. Local tests only are currently authorized.** Virtual passes and USB
readback do not qualify physical installation, handoff or recovery. Do not access
hardware or stage drives without a new authorized task.

Before physical testing, retain an authenticated selector candidate, exact stock
restoration image, full flash/configuration/calibration baseline and a backup of
the test drive. Record exact hashes and distinguish stock/vendor effects from
additional effects caused by AuroraSwitch. Never treat vendor writes also present
stock as selector faults, but investigate changes introduced by our handoff.

When separately authorized:

- Install only the selector through the ordinary stock USB updater.
- Test connected-drive automatic boot, held Shift + Reverse menu, press/release
  behavior, supported firmware audio/buttons and absence/reinsertion of USB.
- Verify changed USB choices persist; automatic/repeat choices do not write.
  Interrupt a choice save: invalid records must open the menu, with no loop.
- Interrupt selector installation during representative erase/program phases.
  Restore through ordinary USB without requiring a functioning application/menu.
- Also restore stock with an unusable selector/application; prove recovery is
  independent of selector buttons or code. Check calibration, internal bootloader,
  reserved data and protection configuration against the retained baseline.
- Exercise vendor settings/workloads relevant to preserving the installed selector
  and compare lasting effects against equivalent stock operation.

Report failures and incomplete tests explicitly. A recoverable crash alone is not
lasting damage; loss of calibration, added wear or ordinary USB recovery is.
