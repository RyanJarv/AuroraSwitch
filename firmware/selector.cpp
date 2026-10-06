// Development-only selector. Default preview has no guest launch path.
#include "aurora.h"
#include "images.hpp"
#include "../support/read_only_image_staging.hpp"
#ifdef SELECTOR_VIRTUAL_TRANSPORT
#include "../support/backed_image_reader.hpp"
// Exact ELF symbol, not a profile copied from another image. Payload is mapped
// by the virtual runner at SDRAM base; this build never initializes real USB.
extern "C" {
volatile daisy_development::BackedImageDescriptor selector_virtual_media{};
}
#endif
#include "mbedtls/sha256.h"
#ifdef SELECTOR_EXPERIMENTAL_HANDOFF
#include "handoff.hpp"
#endif

namespace
{
    daisy::DaisySeed seed;
    daisy::Switch next_button, load_button;
#ifdef SELECTOR_EXPERIMENTAL_HANDOFF
    daisy::Switch launch_button;
#endif
    daisy::LedDriverPca9685<2, true> leds;
    // BOOT_SRAM puts ordinary globals/stack in DTCM, inaccessible to USB DMA.
    // FatFs reads sectors into both FATFS::win and FIL::buf, not just staging.
#ifndef SELECTOR_VIRTUAL_TRANSPORT
    DMA_BUFFER_MEM_SECTION __attribute__((aligned(32)))
    daisy::FatFSInterface selector_filesystem;
    DMA_BUFFER_MEM_SECTION __attribute__((aligned(32))) FIL selector_file;
#endif
    struct LoadDiagnostic
    {
        std::uint32_t attempts, selection, staging_result, fatfs_result;
        std::uint32_t file_size, bytes_read, authenticated;
    };
    volatile LoadDiagnostic load_diagnostic{};
    __attribute__((section(".selector_staging"), aligned(32)))
    std::uint8_t staged[aurora_selector::StagingCapacity()];
    unsigned selected = 0;
    enum class State { Waiting, Selected, Loading, Verified, Error };
    State state = State::Waiting;

#ifdef SELECTOR_VIRTUAL_TRANSPORT
    daisy_development::BackedImageReader MakeReader()
    {
        return daisy_development::BackedImageReader(selector_virtual_media,
            reinterpret_cast<const std::uint8_t*>(0xc0000000U),
            aurora_selector::StagingCapacity());
    }
    bool MediaReady() { return MakeReader().Ready(); }
    void PumpMedia() { MakeReader().Pump(); }
#else
    bool MediaReady() { return aurora::usb.GetReady(); }
    void PumpMedia() { aurora::usb.Process(); }
#endif

    void SetStatus(float r, float g, float b)
    {
        // SDK LED_FREEZE wiring: channels 3, 4, 5.
        leds.SetLed(3, r);
        leds.SetLed(4, g);
        leds.SetLed(5, b);
        // SDK LED_REVERSE indicates selected firmware: FDN blue / spectral green.
        leds.SetLed(1, selected == 1 ? 0.4f : 0.f);
        leds.SetLed(2, selected == 0 ? 0.4f : 0.f);
        leds.SwapBuffersAndTransmit();
    }

#ifndef SELECTOR_VIRTUAL_TRANSPORT
    struct UsbFileReader
    {
        bool Ready() { return aurora::usb.GetReady(); }
        bool Open(const char* path)
        {
            const auto result = f_open(&selector_file, path, FA_READ);
            load_diagnostic.fatfs_result = result;
            return result == FR_OK;
        }
        std::size_t Size()
        {
            load_diagnostic.file_size = f_size(&selector_file);
            return load_diagnostic.file_size;
        }
        bool Read(std::uint8_t* buffer, std::size_t amount, std::size_t& read)
        {
            UINT count = 0;
            const auto result = f_read(&selector_file, buffer, static_cast<UINT>(amount), &count);
            load_diagnostic.fatfs_result = result;
            load_diagnostic.bytes_read = load_diagnostic.bytes_read + count;
            read = count;
            return result == FR_OK;
        }
        void Pump() { aurora::usb.Process(); }
        bool Close()
        {
            const auto result = f_close(&selector_file);
            // Preserve the first operation failure rather than hide it by close.
            if(load_diagnostic.fatfs_result == FR_OK)
                load_diagnostic.fatfs_result = result;
            return result == FR_OK;
        }
    };
#endif

    bool LoadSelected()
    {
        // One read into staging; hash and later handoff consume these same bytes.
        // Never validate a file, close it, and then reopen an unvalidated copy.
#ifdef SELECTOR_VIRTUAL_TRANSPORT
        auto reader = MakeReader();
#else
        UsbFileReader reader;
#endif
        load_diagnostic.attempts = load_diagnostic.attempts + 1U;
        load_diagnostic.selection = selected;
        load_diagnostic.staging_result = 0xffffffffU;
        // Synthetic transport has no FatFs result, including on success.
#ifdef SELECTOR_VIRTUAL_TRANSPORT
        load_diagnostic.fatfs_result = 0xffffffffU;
#else
        load_diagnostic.fatfs_result = FR_OK;
#endif
        load_diagnostic.file_size = 0;
        load_diagnostic.bytes_read = 0;
        load_diagnostic.authenticated = 0;
        const auto& image = aurora_selector::Images[selected];
        const auto result = daisy_development::StageReadOnlyImage(
            reader, image.path, staged, sizeof(staged), image.size);
#ifdef SELECTOR_VIRTUAL_TRANSPORT
        load_diagnostic.file_size = reader.Size();
        load_diagnostic.bytes_read = reader.BytesRead();
#endif
        load_diagnostic.staging_result = static_cast<std::uint32_t>(result);
        std::uint8_t digest[32]{};
        const bool authenticated = result == daisy_development::StagingResult::Staged
               && mbedtls_sha256_ret(staged, image.size, digest, 0) == 0
               && aurora_selector::VerifyImage(staged, image.size, image, digest);
        load_diagnostic.authenticated = authenticated;
        return authenticated;
    }
}

int main()
{
    // Do not use Hardware::Init(): it can persist calibration defaults.
    // No ADC, audio, persistent storage, or low-priority timer is started here.
    // Reuse the pinned System initializer, preserving bootloader clocks/MPU.
    // DaisySeed::Init would reset/reconfigure NOR and initialize unused audio.
    daisy::System::Config system_config;
    system_config.Defaults();
    system_config.skip_clocks = true;
    // Upstream USB host uses DMA. Keep CPU data cache disabled so DMA-written
    // internal staging bytes cannot be replaced by stale cached contents.
    // Re-enable/region-specific cache policy only with a tested coherence contract.
    SCB_DisableDCache();
    system_config.use_dcache = false;
    seed.system.Init(system_config);
    __HAL_RCC_D2SRAM1_CLK_ENABLE();
    __HAL_RCC_D2SRAM2_CLK_ENABLE();
    next_button.Init(seed.GetPin(10), 1000.f); // Reverse
    load_button.Init(seed.GetPin(1), 1000.f);  // Freeze
#ifdef SELECTOR_EXPERIMENTAL_HANDOFF
    launch_button.Init(seed.GetPin(13), 1000.f); // Shift confirms launch
#endif

    daisy::I2CHandle i2c;
    daisy::I2CHandle::Config led_config;
    led_config.periph = daisy::I2CHandle::Config::Peripheral::I2C_1;
    led_config.pin_config = {seed.GetPin(11), seed.GetPin(12)};
    led_config.speed = daisy::I2CHandle::Config::Speed::I2C_1MHZ;
    led_config.mode = daisy::I2CHandle::Config::Mode::I2C_MASTER;
    i2c.Init(led_config);
    leds.Init(i2c, {0x00, 0x01}, aurora::led_dma_buffer_a, aurora::led_dma_buffer_b);
    for(int channel = 0; channel < 32; ++channel)
        leds.SetLed(channel, 0.f);

#ifndef SELECTOR_VIRTUAL_TRANSPORT
    daisy::USBHostHandle::Config usb_config;
    if(aurora::usb.Init(usb_config) != daisy::USBHostHandle::Result::OK)
        state = State::Error;
    daisy::FatFSInterface::Config fs_config;
    fs_config.media = daisy::FatFSInterface::Config::MEDIA_USB;
    if(selector_filesystem.Init(fs_config) != daisy::FatFSInterface::Result::OK)
        state = State::Error;
    if(f_mount(&selector_filesystem.GetUSBFileSystem(),
               selector_filesystem.GetUSBPath(), 0) != FR_OK)
        state = State::Error;

#endif

    std::uint32_t last_control = daisy::System::GetNow();
    while(true)
    {
        PumpMedia();
        const auto now = daisy::System::GetNow();
        if(now == last_control)
            continue;
        last_control = now;
        next_button.Debounce();
        load_button.Debounce();
#ifdef SELECTOR_EXPERIMENTAL_HANDOFF
        launch_button.Debounce();
#endif
        if(!MediaReady() && state != State::Error)
            state = State::Waiting;
        if(MediaReady() && state == State::Waiting)
            state = State::Selected;
        if(next_button.FallingEdge())
        {
            selected ^= 1U;
            state = MediaReady() ? State::Selected : State::Waiting;
        }
        if(load_button.FallingEdge() && MediaReady())
        {
            state = State::Loading;
            SetStatus(0.4f, 0.2f, 0.f);
            state = LoadSelected() ? State::Verified : State::Error;
        }
#ifdef SELECTOR_EXPERIMENTAL_HANDOFF
        if(launch_button.FallingEdge() && state == State::Verified
           && !load_button.FallingEdge() && !next_button.FallingEdge())
        {
#ifdef SELECTOR_VIRTUAL_TRANSPORT
            if(!aurora_selector::VirtualLaunch(seed, selector_virtual_media,
                   aurora_selector::Images[selected], staged))
#else
            if(!aurora_selector::ExperimentalLaunch(
                   seed, aurora::usb, selector_filesystem.GetUSBPath(),
                   aurora_selector::Images[selected], staged))
#endif
                state = State::Error;
        }
#endif
        if(now % 20U == 0U)
        {
            switch(state)
            {
                case State::Waiting: SetStatus(0.f, 0.f, 0.1f); break;
                case State::Selected: SetStatus(0.15f, 0.15f, 0.15f); break;
                case State::Loading: SetStatus(0.4f, 0.2f, 0.f); break;
                case State::Verified: SetStatus(0.f, 0.4f, 0.f); break;
                case State::Error: SetStatus(0.4f, 0.f, 0.f); break;
            }
        }
    }
}
