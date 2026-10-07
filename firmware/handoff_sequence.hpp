#pragma once

namespace aurora_selector
{
    // Shared ordering between firmware and synthetic lifecycle tests.
    // Platform::Fatal and Platform::Jump never return on the real board.
    template<class Platform>
    bool PrepareAndJump(Platform& platform, bool application_replaced = false)
    {
        if(!platform.Validate() || !platform.Unmount())
        {
            // The opt-in flash path may already have replaced the installed app.
            // In that case even a pre-teardown rejection requires reset/recovery.
            if(application_replaced)
                platform.Fatal();
            return false;
        }
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
