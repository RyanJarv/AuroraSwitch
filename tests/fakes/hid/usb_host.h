#pragma once

namespace daisy
{
    struct USBHostHandle
    {
        struct Config
        {
        };
        enum class Result
        {
            OK,
            ERR
        };
        Result Init(Config);
        bool GetReady();
        void Process();
        void Deinit();
    };
} // namespace daisy
