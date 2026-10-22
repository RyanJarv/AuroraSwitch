#pragma once
#include "../firmware/images.hpp"

namespace aurora_selector {
static_assert(SELECTOR_BUNDLE_LIMIT + SELECTOR_UPDATER_BLOCK_SIZE + SELECTOR_CHOICE_SECTOR_SIZE <=
                  SELECTOR_QSPI_BASE + SELECTOR_CHOICE_OFFSET,
              "Leave space for the stock updater's extra EOF erase and inclusive end sector");

// Fixed slots need no reader state. Vectors determine menu presence; only full
// authentication of the staged bytes can authorize launch.
inline const std::uint8_t* InstalledImage(const std::uint8_t* flash, std::size_t size,
                                          std::size_t index) {
    if (!flash || size < BundleEnd || index >= ImageCount)
        return nullptr;

    const auto* bytes = flash + BundleOffsets[index];
    const auto word = [](const std::uint8_t* p) {
        return std::uint32_t(p[0]) | (std::uint32_t(p[1]) << 8) | (std::uint32_t(p[2]) << 16) |
               (std::uint32_t(p[3]) << 24);
    };
    const auto& vectors = Images[index].vectors;
    return word(bytes) == vectors.stack && word(bytes + sizeof(std::uint32_t)) == vectors.reset
               ? bytes
               : nullptr;
}
} // namespace aurora_selector
