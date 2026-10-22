// Hardware adapter for exact-image RAM and QSPI handoff.
#include "handoff.hpp"
#include "handoff_sequence.hpp"
#include "mbedtls/sha256.h"
#include <cstring>
#include "flash_driver.hpp"
#include "../support/resident_image.hpp"

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
struct Platform {
    daisy::DaisySeed& seed;
    const Image& image;
    const std::uint8_t* staged;
    bool Validate() {
        std::uint8_t digest[32]{};
        if (reinterpret_cast<std::uintptr_t>(staged) != StagingAddress ||
            image.size > StagingCapacity() ||
            mbedtls_sha256_ret(staged, image.size, digest, 0) != 0 ||
            !VerifyImage(staged, image.size, image, digest))
            return false;
        if (image.execution == Execution::Sram)
            return true;
        const auto* flash = MappedFlash();
        const auto* application = flash ? flash + QspiApplicationOffset : nullptr;
        return ResidentDirtMatches(application, staged, image.size) &&
               ResidentBootMatches(application, selector_resident_boot_start,
                                   selector_resident_boot_end - selector_resident_boot_start);
    }
    void MaskInterrupts() { __disable_irq(); }
    void ResetBusMasters() {
        // Only reset DMA/USB/selector peripherals, not FMC/SDRAM or QSPI.
        // Force reset rather than merely gating a still-enabled stream.
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
    // Remove selector interrupt/cache state without rebuilding clocks or MPU.
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
        // Preserve the loader's MPU regions. BOOT_SRAM seed.Init skips
        // ConfigureMpu; losing SDRAM cacheability / DMA-buffer policy
        // can leave the target servicing audio but starving its UI.
        __DSB();
        __ISB();
        // Clocks, internal staging SRAM, FMC/QSPI and FPU remain available.
        // Target BOOT_SRAM startup reinitializes runtime peripherals,
        // but deliberately inherits clock and MPU configuration.
    }
    // Copy and read back the stackless routine into reserved SRAM4.
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
            end - begin > limit - ram)
            return false;
        std::memcpy(selector_trampoline_ram, blob, end - begin);
        __DSB();
        __ISB();
        return std::memcmp(selector_trampoline_ram, blob, end - begin) == 0;
    }
    [[noreturn]] void Fatal() {
        // Irreversible teardown: never pretend that the UI can retry.
        __disable_irq();
        while (true) {
            __WFI();
        }
    }
    // Enter SRAM4 to replace selector code without returning to it.
    [[noreturn]] void Jump() {
        using Entry = void (*)(const std::uint8_t*, std::uint8_t*, std::size_t);
        auto entry =
            reinterpret_cast<Entry>(reinterpret_cast<std::uintptr_t>(selector_trampoline_ram) | 1U);
        if (image.execution == Execution::Qspi) {
            // A resident image redirects only the power-on reset word.
            // Explicit Dirt launch uses its authenticated original PC.
            entry(reinterpret_cast<const std::uint8_t*>(SELECTOR_QSPI_APPLICATION_BASE),
                  reinterpret_cast<std::uint8_t*>(image.vectors.reset), 0);
            Fatal();
        }
        entry(staged, reinterpret_cast<std::uint8_t*>(SELECTOR_AXI_SRAM_BASE), image.size);
        Fatal();
    }
};
} // namespace

bool LaunchFirmware(daisy::DaisySeed& seed, unsigned index, const std::uint8_t* staged) {
    // Callers choose an entry, never supply executable metadata themselves.
    if (index >= ImageCount)
        return false;
    const auto& image = Images[index];
    // PrepareAndJump authenticates staged bytes before and after cleanup.
    Platform platform{seed, image, staged};
    return PrepareAndJump(platform);
}
} // namespace aurora_selector
