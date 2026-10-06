#pragma once

namespace aurora_selector
{
    // Shared ordering between firmware and synthetic lifecycle tests.
    // Platform::Fatal and Platform::Jump never return on the real board.
    template<class Platform>
    bool PrepareAndJump(Platform& platform)
    {
        if(!platform.Validate() || !platform.Unmount())
            return false; // no irreversible peripheral teardown yet
        platform.StopMedia();
        platform.MaskInterrupts();
        platform.ResetBusMasters();
        platform.DisableRuntime();
        // Rehash after DMA reset/cache cleaning, not only before teardown.
        if(!platform.Validate() || !platform.InstallTrampoline())
        {
            platform.Fatal();
            return false;
        }
        platform.Jump();
        return true; // synthetic test observer only; real Jump never returns
    }
}
