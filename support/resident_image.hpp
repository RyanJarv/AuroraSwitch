#pragma once
#include "resident_layout.h"
#include "qspi_image_programming.hpp"
#include <cstring>

namespace aurora_selector
{
    inline std::uint32_t ResidentWord(const std::uint8_t* bytes)
    {
        return std::uint32_t(bytes[0]) | (std::uint32_t(bytes[1]) << 8)
            | (std::uint32_t(bytes[2]) << 16) | (std::uint32_t(bytes[3]) << 24);
    }
    inline std::uint32_t ResidentChecksum(const std::uint8_t* bytes, std::size_t size)
    {
        std::uint32_t value = 2166136261U;
        for(std::size_t i = 0; i < size; ++i) value = (value ^ bytes[i]) * 16777619U;
        return value;
    }
    inline bool ResidentPresent(const std::uint8_t* mapped)
    {
        return mapped != nullptr && (ResidentWord(mapped + 4) == SELECTOR_RESIDENT_ENTRY
            || ResidentWord(mapped + (SELECTOR_RESIDENT_METADATA - SELECTOR_RESIDENT_BASE)) == 0x31525341U);
    }
    // The only allowed Dirt transformation changes its reset-vector word.
    // Callers authenticate the complete ORIGINAL staging buffer independently.
    inline bool ResidentDirtMatches(const std::uint8_t* mapped, const std::uint8_t* original,
                                    std::size_t size)
    {
        return mapped && original && size == DirtImageSize
            && ResidentWord(mapped + 4) == SELECTOR_RESIDENT_ENTRY
            && std::memcmp(mapped, original, 4) == 0
            && std::memcmp(mapped + 8, original + 8, size - 8) == 0;
    }
    inline bool ResidentBootMatches(const std::uint8_t* mapped,
                                    const std::uint8_t* boot, std::size_t boot_size)
    {
        if(!mapped || !boot || boot_size == 0 || boot_size > 256
           || std::memcmp(mapped + ((SELECTOR_RESIDENT_ENTRY & ~1U) - SELECTOR_RESIDENT_BASE),
                          boot, boot_size) != 0) return false;
        const auto* metadata = mapped + (SELECTOR_RESIDENT_METADATA - SELECTOR_RESIDENT_BASE);
        const auto size = ResidentWord(metadata + 4);
        if(ResidentWord(metadata) != 0x31525341U || size <= 8 || size > SELECTOR_RESIDENT_CAPACITY
           || ResidentWord(metadata + 8) != ~size) return false;
        const auto* image = mapped + (SELECTOR_RESIDENT_IMAGE - SELECTOR_RESIDENT_BASE);
        return ResidentWord(image) == 0x20020000U
            && (ResidentWord(image + 4) & 1U)
            && ResidentWord(image + 4) > 0x24000000U
            && ResidentWord(image + 4) < 0x24000000U + size
            && ResidentChecksum(image, size) == ResidentWord(metadata + 12);
    }
}
