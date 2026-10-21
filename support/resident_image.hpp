#pragma once
#include "selector_constants.h"
#include "../firmware/images.hpp"
#include "qspi_image_programming.hpp"
#include <cstring>

namespace aurora_selector
{
    constexpr std::uint32_t DirtImageSize = ResidentPayload.size;
    constexpr std::uint32_t QspiApplicationEnd = QspiApplicationOffset
        + ((DirtImageSize + 4095U) & ~4095U);
    constexpr QspiExtent DirtExtent{DirtImageSize, QspiApplicationEnd};
    inline std::uint32_t ResidentWord(const std::uint8_t* bytes)
    {
        return std::uint32_t(bytes[0]) | (std::uint32_t(bytes[1]) << 8)
            | (std::uint32_t(bytes[2]) << 16) | (std::uint32_t(bytes[3]) << 24);
    }
    inline std::uint32_t ResidentChecksum(const std::uint8_t* bytes, std::size_t size)
    {
        std::uint32_t value = SELECTOR_FNV_OFFSET_BASIS;
        for(std::size_t i = 0; i < size; ++i) value = (value ^ bytes[i]) * SELECTOR_FNV_PRIME;
        return value;
    }
    inline bool ResidentPresent(const std::uint8_t* mapped)
    {
        return mapped != nullptr && (ResidentWord(mapped + 4) == SELECTOR_RESIDENT_ENTRY
            || ResidentWord(mapped + (SELECTOR_RESIDENT_METADATA - SELECTOR_RESIDENT_BASE)) == SELECTOR_WRAPPER_MAGIC);
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
        if(ResidentWord(metadata) != SELECTOR_WRAPPER_MAGIC || size <= 8 || size > SELECTOR_RESIDENT_CAPACITY
           || ResidentWord(metadata + 8) != ~size) return false;
        const auto* image = mapped + (SELECTOR_RESIDENT_IMAGE - SELECTOR_RESIDENT_BASE);
        return ResidentWord(image) == SELECTOR_DTCM_END
            && (ResidentWord(image + 4) & 1U)
            && ResidentWord(image + 4) > SELECTOR_AXI_SRAM_BASE
            && ResidentWord(image + 4) < SELECTOR_AXI_SRAM_BASE + size
            && ResidentChecksum(image, size) == ResidentWord(metadata + 12);
    }
}
