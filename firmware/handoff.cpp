#include "handoff.hpp"
#include "handoff_sequence.hpp"
#include "fatfs.h"
#include "mbedtls/sha256.h"
#include <cstring>
#ifdef SELECTOR_VIRTUAL_TRANSPORT
#include "../support/backed_image_reader.hpp"
#endif

extern "C"
{
    extern const std::uint8_t selector_copy_jump_blob_start[];
    extern const std::uint8_t selector_copy_jump_end[];
    extern std::uint8_t selector_trampoline_ram[];
    extern std::uint8_t selector_trampoline_limit[];
}

namespace aurora_selector
{
    namespace
    {
        struct UsbTransport
        {
            daisy::USBHostHandle& usb;
            const char* path;
            bool Unmount() { return f_mount(nullptr, path, 0) == FR_OK; }
            void Stop() { usb.Deinit(); }
        };
#ifdef SELECTOR_VIRTUAL_TRANSPORT
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
        template<class Transport> struct Platform
        {
            daisy::DaisySeed& seed;
            Transport& transport;
            const Image& image;
            const std::uint8_t* staged;

            bool Validate()
            {
                std::uint8_t digest[32]{};
                return reinterpret_cast<std::uintptr_t>(staged) == StagingAddress
                       && image.size <= StagingCapacity()
                       && mbedtls_sha256_ret(staged, image.size, digest, 0) == 0
                       && VerifyImage(staged, image.size, image, digest);
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
            bool InstallTrampoline()
            {
                const auto begin = reinterpret_cast<std::uintptr_t>(selector_copy_jump_blob_start);
                const auto end = reinterpret_cast<std::uintptr_t>(selector_copy_jump_end);
                const auto ram = reinterpret_cast<std::uintptr_t>(selector_trampoline_ram);
                const auto limit = reinterpret_cast<std::uintptr_t>(selector_trampoline_limit);
                if(end <= begin || ram != 0x38000000U || limit <= ram
                   || end - begin > limit - ram)
                    return false;
                std::memcpy(selector_trampoline_ram, selector_copy_jump_blob_start, end - begin);
                __DSB();
                __ISB();
                return std::memcmp(selector_trampoline_ram, selector_copy_jump_blob_start,
                                   end - begin) == 0;
            }
            [[noreturn]] void Fatal()
            {
                // Irreversible teardown: never pretend that the UI can retry.
                while(true) { __WFI(); }
            }
            [[noreturn]] void Jump()
            {
                using Entry = void (*)(const std::uint8_t*, std::uint8_t*, std::size_t);
                auto entry = reinterpret_cast<Entry>(
                    reinterpret_cast<std::uintptr_t>(selector_trampoline_ram) | 1U);
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
        return PrepareAndJump(platform);
    }
#ifdef SELECTOR_VIRTUAL_TRANSPORT
    bool VirtualLaunch(daisy::DaisySeed& seed,
                       volatile daisy_development::BackedImageDescriptor& descriptor,
                       const Image& image, const std::uint8_t* staged)
    {
        BackedTransport transport{descriptor};
        Platform<BackedTransport> platform{seed, transport, image, staged};
        return PrepareAndJump(platform);
    }
#endif
}
