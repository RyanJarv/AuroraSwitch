#pragma once
#include "daisy_seed.h"
#include "../support/selector_constants.h"

namespace aurora_selector {
inline const std::uint8_t* MappedFlash() {
    if ((QUADSPI->CR & QUADSPI_CR_EN) == 0 ||
        (QUADSPI->CCR & QUADSPI_CCR_FMODE) != QUADSPI_CCR_FMODE)
        return nullptr;
    __DSB();
    return reinterpret_cast<const std::uint8_t*>(SELECTOR_QSPI_BASE);
}
// Same pin/device configuration as pinned DaisySeed, without audio Init.
struct ChoiceFlashDriver {
    daisy::QSPIHandle& qspi;
    bool initialized = false;
    bool Initialize() {
        if (initialized && qspi.GetStatus() == daisy::QSPIHandle::GOOD && MappedFlash())
            return true;
        daisy::QSPIHandle::Config config{};
        config.device = daisy::QSPIHandle::Config::IS25LP064A;
        // Initialization is only requested for a changed choice. Enter write
        // mode directly; WritePage restores mapped reads after the program.
        config.mode = daisy::QSPIHandle::Config::INDIRECT_POLLING;
        config.pin_config.io0 = dsy_pin(DSY_GPIOF, 8);
        config.pin_config.io1 = dsy_pin(DSY_GPIOF, 9);
        config.pin_config.io2 = dsy_pin(DSY_GPIOF, 7);
        config.pin_config.io3 = dsy_pin(DSY_GPIOF, 6);
        config.pin_config.clk = dsy_pin(DSY_GPIOF, 10);
        config.pin_config.ncs = dsy_pin(DSY_GPIOG, 6);
        initialized = qspi.Init(config) == daisy::QSPIHandle::OK;
        return initialized;
    }
    bool EraseSector(std::uint32_t offset) {
        if (offset != SELECTOR_CHOICE_OFFSET &&
            offset != SELECTOR_CHOICE_OFFSET + SELECTOR_CHOICE_SECTOR_SIZE)
            return false;
        return qspi.EraseSector(offset) == daisy::QSPIHandle::OK;
    }
    bool WritePage(std::uint32_t offset, std::size_t size, const std::uint8_t* bytes) {
        if (offset < SELECTOR_CHOICE_OFFSET ||
            offset >= SELECTOR_CHOICE_OFFSET + 2 * SELECTOR_CHOICE_SECTOR_SIZE ||
            offset % SELECTOR_CHOICE_RECORD_STRIDE || size != SELECTOR_CHOICE_RECORD_SIZE)
            return false;
        // Use a checked single-page write; the SDK's bulk writer drops page errors.
        return qspi.WritePage(offset, size, const_cast<std::uint8_t*>(bytes)) ==
               daisy::QSPIHandle::OK;
    }
    const std::uint8_t* MappedData() { return MappedFlash(); }
};
} // namespace aurora_selector
