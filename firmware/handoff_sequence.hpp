#pragma once

namespace aurora_selector {
// Refuse invalid bytes while the menu can still run. After cleanup begins, a
// failure must stop: the selector no longer owns working peripherals/interrupts.
template <class Platform> bool PrepareAndJump(Platform& platform) {
    if (!platform.Validate())
        return false;
    platform.MaskInterrupts();
    platform.ResetBusMasters();
    platform.DisableRuntime();
    // Cleanup may have changed RAM. Check the actual staged bytes again.
    if (!platform.Validate() || !platform.InstallTrampoline()) {
        platform.Fatal();
        return false;
    }
    platform.Jump();
    return true; // Host observer only; the board jump never returns.
}
} // namespace aurora_selector
