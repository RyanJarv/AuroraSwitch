#pragma once

// Structural checks for a caller-pinned application base, not image identity.
#include <cstddef>
#include <cstdint>

namespace daisy_development
{
    // First two vector words; reset retains the Cortex-M Thumb bit.
    struct SramImageVectors
    {
        std::uint32_t stack;
        std::uint32_t reset;
    };

    // Structural bounds only: the caller must separately authenticate the whole
    // image and establish the physical boot/handoff contract. No launch authority.
    // Allocation-free so the same check can run in a selector firmware.
    inline bool ReadImageVectors(const std::uint8_t* bytes,
                                    std::size_t size,
                                    std::size_t capacity,
                                    std::uint32_t base,
                                    SramImageVectors& result)
    {
        if(bytes == nullptr || size < 16U || size > capacity
           || capacity > 512U * 1024U)
            return false;
        const auto word = [bytes](std::size_t offset) {
            return std::uint32_t(bytes[offset])
                   | (std::uint32_t(bytes[offset + 1U]) << 8)
                   | (std::uint32_t(bytes[offset + 2U]) << 16)
                   | (std::uint32_t(bytes[offset + 3U]) << 24);
        };
        const auto stack = word(0U);
        const auto reset = word(4U);
        const auto entry = reset & ~1U;
        if(stack <= 0x20000000U || stack > 0x20020000U
           || (stack & 7U) != 0U || (reset & 1U) == 0U
           || entry < base
           || std::uint64_t(entry) + 2U > std::uint64_t(base) + size)
            return false;
        result = {stack, reset};
        return true;
    }

    // Existing SRAM callers retain their fixed-base structural contract.
    inline bool ReadSramImageVectors(const std::uint8_t* bytes, std::size_t size,
                                    std::size_t capacity, SramImageVectors& result)
    {
        return ReadImageVectors(bytes, size, capacity, 0x24000000U, result);
    }
}
