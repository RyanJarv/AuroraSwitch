// Hardware adapter for exact-image RAM and QSPI handoff.
#include "handoff.hpp"
#include "handoff_sequence.hpp"
#include "fatfs.h"
#include "mbedtls/sha256.h"
#include <cstring>
#include "../support/qspi_image_programming.hpp"
extern "C" {
volatile aurora_selector::QspiDiagnostic selector_qspi_diagnostic{};
}
#ifdef SELECTOR_VIRTUAL_TRANSPORT
#include "../support/backed_image_reader.hpp"
#endif

extern "C"
{
    extern const std::uint8_t selector_copy_jump_blob_start[];
    extern const std::uint8_t selector_copy_jump_end[];
    extern std::uint8_t selector_trampoline_ram[];
    extern std::uint8_t selector_trampoline_limit[];
    extern const std::uint8_t selector_qspi_jump_blob_start[];
    extern const std::uint8_t selector_qspi_jump_end[];
}

namespace aurora_selector
{
    namespace
    {
        // Same pin/device configuration as pinned DaisySeed, without audio Init.
        struct QspiDriver
        {
            daisy::QSPIHandle& qspi;
            bool Initialize()
            {
                daisy::QSPIHandle::Config config{};
                config.device = daisy::QSPIHandle::Config::IS25LP064A;
                config.mode = daisy::QSPIHandle::Config::MEMORY_MAPPED;
                config.pin_config.io0 = dsy_pin(DSY_GPIOF, 8);
                config.pin_config.io1 = dsy_pin(DSY_GPIOF, 9);
                config.pin_config.io2 = dsy_pin(DSY_GPIOF, 7);
                config.pin_config.io3 = dsy_pin(DSY_GPIOF, 6);
                config.pin_config.clk = dsy_pin(DSY_GPIOF, 10);
                config.pin_config.ncs = dsy_pin(DSY_GPIOG, 6);
                return qspi.Init(config) == daisy::QSPIHandle::OK;
            }
            bool EraseSector(std::uint32_t offset)
            { return qspi.EraseSector(offset) == daisy::QSPIHandle::OK; }
            bool WritePage(std::uint32_t offset, std::size_t size, const std::uint8_t* bytes)
            {
                // Bulk Write discards page errors in this pinned SDK.
                return qspi.WritePage(offset, size, const_cast<std::uint8_t*>(bytes))
                       == daisy::QSPIHandle::OK;
            }
            const std::uint8_t* MappedData()
            {
                if(qspi.GetConfig().mode != daisy::QSPIHandle::Config::MEMORY_MAPPED
                   || qspi.GetStatus() != daisy::QSPIHandle::GOOD)
                    return nullptr;
                __DSB();
                // Selector initializes with D-cache disabled; preserve that policy.
                return static_cast<const std::uint8_t*>(qspi.GetData(QspiApplicationOffset));
            }
        };
        // Release the selector's real filesystem and USB host before replacement.
        struct UsbTransport
        {
            daisy::USBHostHandle& usb;
            const char* path;
            bool Unmount() { return f_mount(nullptr, path, 0) == FR_OK; }
            void Stop() { usb.Deinit(); }
        };
#ifdef SELECTOR_VIRTUAL_TRANSPORT
        // Test-media adapter; shares cleanup ordering without claiming USB behavior.
        struct BackedTransport
        {
            volatile daisy_development::BackedImageDescriptor& descriptor;
            bool Unmount()
            {
                // Synthetic media withdrawal, not a successful FatFs unmount.
                return daisy_development::BackedImageReader(descriptor,
                    reinterpret_cast<const std::uint8_t*>(0xc0000000U),
                    StagingCapacity()).Ready();
            }
            void Stop() { descriptor.ready = 0; }
        };
#endif
        // Bind shared ordering to hardware while preserving bootloader-owned state.
        template<class Transport> struct Platform
        {
            daisy::DaisySeed& seed;
            Transport& transport;
            const Image& image;
            const std::uint8_t* staged;
            bool qspi_prepared = false;

            bool PrepareTarget()
            {
                if(image.execution == Execution::Sram)
                    return true;
                // Only an exact QSPI catalog value can authorize programming.
                // Vector/hash validation alone must not admit caller-made images.
                if(!IsReviewedQspiImage(image) || !Validate())
                    return false;
                QspiDriver driver{seed.qspi};
                const auto result = ProgramQspiImage(driver, staged, image.size,
                    [this] { return Validate(); }, selector_qspi_diagnostic,
                    {image.size, QspiApplicationOffset
                        + static_cast<std::uint32_t>((image.size + 4095U) & ~std::size_t(4095U))});
                if(result != QspiResult::Ready)
                {
                    // Any attempted erase may have destroyed the installed selector.
                    // Reset with the root selector USB present; never retry the UI.
                    if(selector_qspi_diagnostic.erase_attempts != 0)
                        Fatal();
                    return false;
                }
                qspi_prepared = true;
                return true;
            }

            // Rehash the same fixed staging buffer before and after cleanup.
            bool Validate()
            {
                std::uint8_t digest[32]{};
                const bool staged_valid = reinterpret_cast<std::uintptr_t>(staged) == StagingAddress
                       && image.size <= StagingCapacity()
                       && mbedtls_sha256_ret(staged, image.size, digest, 0) == 0
                       && VerifyImage(staged, image.size, image, digest);
                if(staged_valid && qspi_prepared)
                {
                    QspiDriver driver{seed.qspi};
                    const auto* mapped = driver.MappedData();
                    return mapped != nullptr && std::memcmp(mapped, staged, image.size) == 0;
                }
                return staged_valid;
            }
            bool Unmount() { return transport.Unmount(); }
            void StopMedia() { transport.Stop(); }
            void MaskInterrupts() { __disable_irq(); }
            void ResetBusMasters()
            {
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
            void DisableRuntime()
            {
                seed.DeInit(); // upstream DMA/timer deinit + cache clean/disable
                SysTick->CTRL = 0;
                SysTick->LOAD = 0;
                SysTick->VAL = 0;
                for(unsigned bank = 0; bank < 8; ++bank)
                {
                    NVIC->ICER[bank] = 0xffffffffU;
                    NVIC->ICPR[bank] = 0xffffffffU;
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
            bool InstallTrampoline()
            {
                const std::uint8_t* blob = selector_copy_jump_blob_start;
                const std::uint8_t* blob_end = selector_copy_jump_end;
                if(image.execution == Execution::Qspi)
                {
                    blob = selector_qspi_jump_blob_start;
                    blob_end = selector_qspi_jump_end;
                }
                const auto begin = reinterpret_cast<std::uintptr_t>(blob);
                const auto end = reinterpret_cast<std::uintptr_t>(blob_end);
                const auto ram = reinterpret_cast<std::uintptr_t>(selector_trampoline_ram);
                const auto limit = reinterpret_cast<std::uintptr_t>(selector_trampoline_limit);
                if(end <= begin || ram != 0x38000000U || limit <= ram
                   || end - begin > limit - ram)
                    return false;
                std::memcpy(selector_trampoline_ram, blob, end - begin);
                __DSB();
                __ISB();
                return std::memcmp(selector_trampoline_ram, blob,
                                   end - begin) == 0;
            }
            [[noreturn]] void Fatal()
            {
                // Irreversible teardown: never pretend that the UI can retry.
                __disable_irq();
                while(true) { __WFI(); }
            }
            // Enter SRAM4 to replace selector code without returning to it.
            [[noreturn]] void Jump()
            {
                using Entry = void (*)(const std::uint8_t*, std::uint8_t*, std::size_t);
                auto entry = reinterpret_cast<Entry>(
                    reinterpret_cast<std::uintptr_t>(selector_trampoline_ram) | 1U);
                if(image.execution == Execution::Qspi)
                {
                    entry(reinterpret_cast<const std::uint8_t*>(0x90040000U), nullptr, 0);
                    Fatal();
                }
                entry(staged, reinterpret_cast<std::uint8_t*>(0x24000000U), image.size);
                Fatal();
            }
        };
    }

    bool ExperimentalLaunch(daisy::DaisySeed& seed, daisy::USBHostHandle& usb,
                            const char* media_path, const Image& image,
                            const std::uint8_t* staged)
    {
        UsbTransport transport{usb, media_path};
        Platform<UsbTransport> platform{seed, transport, image, staged};
        if(!platform.PrepareTarget())
            return false;
        return PrepareAndJump(platform, platform.qspi_prepared);
    }
#ifdef SELECTOR_VIRTUAL_TRANSPORT
    bool VirtualLaunch(daisy::DaisySeed& seed,
                       volatile daisy_development::BackedImageDescriptor& descriptor,
                       const Image& image, const std::uint8_t* staged)
    {
        BackedTransport transport{descriptor};
        Platform<BackedTransport> platform{seed, transport, image, staged};
        if(!platform.PrepareTarget())
            return false;
        return PrepareAndJump(platform, platform.qspi_prepared);
    }
#endif
}
