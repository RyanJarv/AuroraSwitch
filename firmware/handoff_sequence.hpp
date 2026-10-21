#pragma once

namespace aurora_selector {
// Check the chosen firmware, stop the selector, then start that firmware.
// Tests use a fake board with the same step order. On hardware, Fatal stops
// execution and Jump transfers control; neither returns.
template <class Platform>
bool PrepareAndJump(Platform& platform, bool application_replaced = false) {
    if (!platform.Validate() || !platform.Unmount()) {
        // Flash may already contain a different application. If so, returning
        // to the menu is unsafe even though peripheral cleanup has not begun.
        if (application_replaced) {
            platform.Fatal();
        }
        return false;
    }
    platform.StopMedia();
    platform.MaskInterrupts();
    platform.ResetBusMasters();
    platform.DisableRuntime();
    // Verify again after stopping memory-writing peripherals and flushing
    // caches: the first check cannot detect damage caused by cleanup.
    if (!platform.Validate() || !platform.InstallTrampoline()) {
        platform.Fatal();
        return false;
    }
    platform.Jump();
    return true; // synthetic test observer only; real Jump never returns
}
} // namespace aurora_selector
