#pragma once
// Inspectable operation timing; it does not enforce a storage deadline.
#include <cstdint>

namespace daisy_development
{
    // Millisecond diagnostic only; no timeout or execution authority. Modular
    // subtraction tolerates one clock wrap, not operations lasting >= 2^32 ms.
    struct OperationTiming
    {
        std::uint32_t started_ms = 0;
        std::uint32_t elapsed_ms = 0;
        std::uint32_t maximum_ms = 0;
        std::uint32_t completed = 0;

        // Reset this attempt while retaining the session maximum.
        void Begin(std::uint32_t now) volatile
        {
            completed = 0;
            started_ms = now;
            elapsed_ms = 0;
        }
        void Finish(std::uint32_t now) volatile
        {
            const auto duration = now - started_ms;
            elapsed_ms = duration;
            if(duration > maximum_ms) maximum_ms = duration;
            completed = 1;
        }
    };
}
