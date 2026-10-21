// Start an authenticated firmware after shutting down the selector safely.
// RAM firmware replaces the running selector; QSPI firmware runs from flash.
#include "handoff.hpp"
#include "handoff_sequence.hpp"
#include "mbedtls/sha256.h"
#include <cstring>
#include "../support/qspi_image_programming.hpp"
#include "../support/resident_image.hpp"
extern "C" {
volatile aurora_selector::QspiDiagnostic selector_qspi_diagnostic{};
}

extern "C" {
extern const std::uint8_t selector_copy_jump_blob_start[];
extern const std::uint8_t selector_copy_jump_end[];
extern std::uint8_t selector_trampoline_ram[];
extern std::uint8_t selector_trampoline_limit[];
extern const std::uint8_t selector_qspi_jump_blob_start[];
extern const std::uint8_t selector_qspi_jump_end[];
extern const std::uint8_t selector_resident_boot_start[];
extern const std::uint8_t selector_resident_boot_end[];
}

namespace aurora_selector {
namespace {
// Reuse flash access set up by the bootloader when the resident selector
// starts. Initializing QSPI again would write flash configuration
// registers even when no firmware bytes need changing.
const std::uint8_t* InheritedQspiData() {
    if ((QUADSPI->CR & QUADSPI_CR_EN) == 0 ||
        (QUADSPI->CCR & QUADSPI_CCR_FMODE) != QUADSPI_CCR_FMODE) {
        return nullptr;
    }
    __DSB();
    return reinterpret_cast<const std::uint8_t*>(SELECTOR_QSPI_APPLICATION_BASE);
}

// Configure only the flash interface, using the board SDK's pin mapping.
// Full DaisySeed initialization would also start unrelated audio hardware.
struct QspiDriver {
    daisy::QSPIHandle& qspi;
    bool initialized = false;

    bool Initialize() {
        if (initialized) {
            return true;
        }
        daisy::QSPIHandle::Config config{};
        config.device = daisy::QSPIHandle::Config::IS25LP064A;
        config.mode = daisy::QSPIHandle::Config::MEMORY_MAPPED;
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
        return qspi.EraseSector(offset) == daisy::QSPIHandle::OK;
    }

    bool WritePage(std::uint32_t offset, std::size_t size, const std::uint8_t* bytes) {
        // Write one page at a time so a failed write is reported.
        // The pinned SDK's bulk Write method discards individual page errors.
        return qspi.WritePage(offset, size, const_cast<std::uint8_t*>(bytes)) ==
               daisy::QSPIHandle::OK;
    }

    const std::uint8_t* MappedData() {
        if (qspi.GetConfig().mode != daisy::QSPIHandle::Config::MEMORY_MAPPED ||
            qspi.GetStatus() != daisy::QSPIHandle::GOOD) {
            return nullptr;
        }
        __DSB();
        // Selector initializes with D-cache disabled; preserve that policy.
        return static_cast<const std::uint8_t*>(qspi.GetData(QspiApplicationOffset));
    }
};

// Connect the shared launch steps to the board. Keep the bootloader's
// clock and memory configuration that the next firmware relies on.
struct Platform {
    daisy::DaisySeed& seed;
    MediaTransport& transport;
    const Image& image;
    const std::uint8_t* staged;
    bool qspi_prepared = false;
    bool resident = false;
    bool application_replaced = false;

    bool PrepareTarget() {
        if (image.execution == Execution::Sram) {
            return true;
        }
        // Only an exact QSPI catalog value can authorize programming.
        // Vector/hash validation alone must not admit caller-made images.
        if (!IsReviewedQspiImage(image) || !Validate()) {
            return false;
        }
        QspiDriver driver{seed.qspi};
        const auto* mapped = InheritedQspiData();
        if (mapped == nullptr) {
            if (!driver.Initialize()) {
                return false;
            }
            mapped = driver.MappedData();
        }
        if (ResidentPresent(mapped)) {
            // A local composite stays installed. Never repair it by
            // erasing Dirt's vectors or the resident dispatcher.
            selector_qspi_diagnostic.stage = selector_qspi_diagnostic.erase_attempts = 0;
            selector_qspi_diagnostic.pages = selector_qspi_diagnostic.verified = 0;
            selector_qspi_diagnostic.result =
                static_cast<std::uint32_t>(QspiResult::ReadbackFailed);
            if (!ResidentDirtMatches(mapped, staged, image.size) ||
                !ResidentBootMatches(mapped, selector_resident_boot_start,
                                     selector_resident_boot_end - selector_resident_boot_start) ||
                !Validate()) {
                return false;
            }
            resident = qspi_prepared = true;
            selector_qspi_diagnostic.verified = image.size;
            selector_qspi_diagnostic.stage = 5;
            selector_qspi_diagnostic.result = static_cast<std::uint32_t>(QspiResult::Ready);
            return true;
        }
        const auto result = ProgramQspiImage(
            driver, staged, image.size, [this] { return Validate(); }, selector_qspi_diagnostic,
            {image.size, QspiApplicationOffset + static_cast<std::uint32_t>((image.size + 4095U) &
                                                                            ~std::size_t(4095U))});
        application_replaced = selector_qspi_diagnostic.erase_attempts != 0;
        if (result != QspiResult::Ready) {
            // Any attempted erase may have destroyed the installed selector.
            // Reset with the root selector USB present; never retry the UI.
            if (selector_qspi_diagnostic.erase_attempts != 0) {
                Fatal();
            }
            return false;
        }
        qspi_prepared = true;
        return true;
    }

    // Check the bytes that will actually run, both before and after cleanup.
    // Peripheral shutdown must not silently damage the staged firmware.
    bool Validate() {
        std::uint8_t digest[32]{};
        const bool staged_valid = reinterpret_cast<std::uintptr_t>(staged) == StagingAddress &&
                                  image.size <= StagingCapacity() &&
                                  mbedtls_sha256_ret(staged, image.size, digest, 0) == 0 &&
                                  VerifyImage(staged, image.size, image, digest);
        if (staged_valid && qspi_prepared) {
            QspiDriver driver{seed.qspi};
            const auto* mapped = resident ? InheritedQspiData() : driver.MappedData();
            return resident ? ResidentDirtMatches(mapped, staged, image.size) &&
                                  ResidentBootMatches(mapped, selector_resident_boot_start,
                                                      selector_resident_boot_end -
                                                          selector_resident_boot_start)
                            : mapped != nullptr && std::memcmp(mapped, staged, image.size) == 0;
        }
        return staged_valid;
    }

    bool Unmount() { return transport.Unmount(); }

    void StopMedia() { transport.Stop(); }

    void MaskInterrupts() { __disable_irq(); }

    void ResetBusMasters() {
        // Stop peripherals that can still write memory after the selector exits.
        // Reset their DMA engines; disabling a clock alone can leave a
        // transfer armed. Keep external RAM (FMC/SDRAM) and flash (QSPI)
        // available because the next firmware may need them.
        __HAL_RCC_DMA1_FORCE_RESET();
        __HAL_RCC_DMA2_FORCE_RESET();
        __HAL_RCC_MDMA_FORCE_RESET();
        __HAL_RCC_BDMA_FORCE_RESET();
        __HAL_RCC_USB1_OTG_HS_FORCE_RESET();
        __HAL_RCC_I2C1_FORCE_RESET();
        __HAL_RCC_SAI1_FORCE_RESET();
        __HAL_RCC_SAI2_FORCE_RESET();
        __DSB();
        __HAL_RCC_DMA1_RELEASE_RESET();
        __HAL_RCC_DMA2_RELEASE_RESET();
        __HAL_RCC_MDMA_RELEASE_RESET();
        __HAL_RCC_BDMA_RELEASE_RESET();
        __HAL_RCC_USB1_OTG_HS_RELEASE_RESET();
        __HAL_RCC_I2C1_RELEASE_RESET();
        __HAL_RCC_SAI1_RELEASE_RESET();
        __HAL_RCC_SAI2_RELEASE_RESET();
        __DSB();
    }

    // Stop selector interrupts and flush cached writes before the next firmware
    // takes over. Preserve clocks and the memory protection unit (MPU).
    void DisableRuntime() {
        seed.DeInit(); // upstream DMA/timer deinit + cache clean/disable
        SysTick->CTRL = 0;
        SysTick->LOAD = 0;
        SysTick->VAL = 0;
        for (unsigned bank = 0; bank < 8; ++bank) {
            NVIC->ICER[bank] = SELECTOR_NVIC_ALL_INTERRUPTS;
            NVIC->ICPR[bank] = SELECTOR_NVIC_ALL_INTERRUPTS;
        }
        SCB->ICSR = SCB_ICSR_PENDSTCLR_Msk | SCB_ICSR_PENDSVCLR_Msk;
        // Keep the loader's memory access/cache rules. RAM firmware does not
        // rebuild these MPU regions during seed.Init. Removing them can
        // leave audio running while the controls stop responding.
        __DSB();
        __ISB();
        // Clocks, internal staging SRAM, FMC/QSPI and FPU remain available.
        // Target BOOT_SRAM startup reinitializes runtime peripherals,
        // but deliberately inherits clock and MPU configuration.
    }

    // Put the final jump routine in separate RAM so it survives replacing
    // the selector. Read it back before trusting it (SRAM4, no stack use).
    bool InstallTrampoline() {
        const std::uint8_t* blob = selector_copy_jump_blob_start;
        const std::uint8_t* blob_end = selector_copy_jump_end;
        if (image.execution == Execution::Qspi) {
            blob = selector_qspi_jump_blob_start;
            blob_end = selector_qspi_jump_end;
        }
        const auto begin = reinterpret_cast<std::uintptr_t>(blob);
        const auto end = reinterpret_cast<std::uintptr_t>(blob_end);
        const auto ram = reinterpret_cast<std::uintptr_t>(selector_trampoline_ram);
        const auto limit = reinterpret_cast<std::uintptr_t>(selector_trampoline_limit);
        if (end <= begin || ram != SELECTOR_TRAMPOLINE_BASE || limit <= ram ||
            end - begin > limit - ram) {
            return false;
        }
        std::memcpy(selector_trampoline_ram, blob, end - begin);
        __DSB();
        __ISB();
        return std::memcmp(selector_trampoline_ram, blob, end - begin) == 0;
    }

    [[noreturn]] void Fatal() {
        // Cleanup or flash replacement has gone too far to return to the menu.
        // Stop here; restarting with recovery media is the safe next step.
        __disable_irq();
        while (true) {
            __WFI();
        }
    }

    // Run the separate jump routine. It must never return to selector code
    // that may already have been overwritten.
    [[noreturn]] void Jump() {
        using Entry = void (*)(const std::uint8_t*, std::uint8_t*, std::size_t);
        auto entry =
            reinterpret_cast<Entry>(reinterpret_cast<std::uintptr_t>(selector_trampoline_ram) | 1U);
        if (image.execution == Execution::Qspi) {
            // A resident image redirects only the power-on reset word.
            // Explicit Dirt launch uses its authenticated original PC.
            entry(reinterpret_cast<const std::uint8_t*>(SELECTOR_QSPI_APPLICATION_BASE),
                  resident ? reinterpret_cast<std::uint8_t*>(image.vectors.reset) : nullptr, 0);
            Fatal();
        }
        entry(staged, reinterpret_cast<std::uint8_t*>(SELECTOR_AXI_SRAM_BASE), image.size);
        Fatal();
    }
};
} // namespace

bool ExperimentalLaunch(daisy::DaisySeed& seed, MediaTransport& transport, const Image& image,
                        const std::uint8_t* staged) {
    Platform platform{seed, transport, image, staged};
    if (!platform.PrepareTarget()) {
        return false;
    }
    return PrepareAndJump(platform, platform.application_replaced);
}
} // namespace aurora_selector
