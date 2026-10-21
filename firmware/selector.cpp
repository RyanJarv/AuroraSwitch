// Panel menu and read-only loading; handoff.cpp owns irreversible launch.
#include "aurora.h"
#include "images.hpp"
#include "selector_controller.hpp"
#include "../support/boot_choice.hpp"
#include "../support/read_only_image_staging.hpp"
#include "../support/media_initialization.hpp"
#include "../support/operation_timing.hpp"
#include "file_media.hpp"
#include "mbedtls/sha256.h"
#include "handoff.hpp"

namespace
{
    daisy::DaisySeed seed;
    daisy::Switch next_button, load_button;
    daisy::Switch category_button;
    daisy::LedDriverPca9685<2, true> leds;
    aurora_selector::MediaTransport transport;
    using aurora_selector::LoadDiagnostic;
    volatile LoadDiagnostic load_diagnostic{};
    volatile daisy_development::OperationTiming discovery_timing{};
    __attribute__((section(".selector_staging"),
                   aligned(32))) std::uint8_t staged[aurora_selector::StagingCapacity()];

    struct SelectorMedia
    {
        bool Ready();
        void BeginDiscovery();
        void EndDiscovery();
        bool Authenticate(unsigned index);
        bool Launch(unsigned index);
        int BootChoice();
        bool Remember(unsigned index);
    };

    SelectorMedia media;
    aurora_selector::SelectorController<SelectorMedia> controller(media);
    using State = aurora_selector::SelectorController<SelectorMedia>::State;
    volatile daisy_development::MediaInitialization media_initialization =
        daisy_development::MediaInitialization::Ready;

    // Freeze shows load status; Reverse shows the current catalog selection.
    void SetStatus(float r, float g, float b)
    {
        // SDK LED_FREEZE wiring: channels 3, 4, 5.
        leds.SetLed(3, r);
        leds.SetLed(4, g);
        leds.SetLed(5, b);
        // Selection colors belong to the exact catalog, not two special indices.
        const auto color = controller.Menu().HasSelection()
            ? aurora_selector::Images[controller.Selected()].menu_color
            : std::array<float, 3>{0.f, 0.f, 0.f};
        for (unsigned component = 0; component < color.size(); ++component)
        {
            leds.SetLed(component, color[component]);
        }
        // SDK arc wiring is non-sequential. One lit arc shows the group's position.
        constexpr unsigned arcs[6][3] = {{6, 7, 8},    {9, 10, 11},  {16, 17, 18},
                                         {12, 13, 14}, {22, 23, 24}, {28, 29, 30}};
        for (unsigned position = 0; position < 6; ++position)
        {
            for (unsigned component = 0; component < 3; ++component)
            {
                leds.SetLed(
                    arcs[position][component],
                    controller.Menu().HasSelection() && position == controller.Menu().Position()
                        ? aurora_selector::CategoryColors[controller.Menu().Category()][component]
                        : 0.f);
            }
        }
        leds.SwapBuffersAndTransmit();
    }

    // Used by discovery and Freeze; success authenticates the current RAM copy.
    bool AuthenticateFile(unsigned index)
    {
        load_diagnostic.timing.Begin(daisy::System::GetNow());
        // One read into staging; hash and later handoff consume these same bytes.
        // Never validate a file, close it, and then reopen an unvalidated copy.
        aurora_selector::FileReader reader{transport, load_diagnostic};
        load_diagnostic.attempts = load_diagnostic.attempts + 1U;
        load_diagnostic.selection = index;
        load_diagnostic.staging_result = SELECTOR_DIAGNOSTIC_UNAVAILABLE;
        load_diagnostic.fatfs_result = transport.SuccessResult();
        load_diagnostic.file_size = 0;
        load_diagnostic.bytes_read = 0;
        load_diagnostic.authenticated = 0;
        const auto& image = aurora_selector::Images[index];
        const auto result = daisy_development::StageReadOnlyImage(reader, image.path, staged,
                                                                  sizeof(staged), image.size);
        load_diagnostic.staging_result = static_cast<std::uint32_t>(result);
        std::uint8_t digest[32]{};
        const bool authenticated = result == daisy_development::StagingResult::Staged
            && mbedtls_sha256_ret(staged, image.size, digest, 0) == 0
            && aurora_selector::VerifyImage(staged, image.size, image, digest);
        load_diagnostic.authenticated = authenticated;
        load_diagnostic.timing.Finish(daisy::System::GetNow());
        return authenticated;
    }

    bool SelectorMedia::Ready()
    {
        return transport.Ready();
    }

    void SelectorMedia::BeginDiscovery()
    {
        discovery_timing.Begin(daisy::System::GetNow());
    }

    void SelectorMedia::EndDiscovery()
    {
        discovery_timing.Finish(daisy::System::GetNow());
    }

    bool SelectorMedia::Authenticate(unsigned index)
    {
        return AuthenticateFile(index);
    }

    int SelectorMedia::BootChoice()
    {
        aurora_selector::FileBootStore store{transport};
        return aurora_selector::ReadBootChoice(store).index;
    }

    bool SelectorMedia::Remember(unsigned index)
    {
        aurora_selector::FileBootStore store{transport};
        return aurora_selector::RememberBootChoice(store, index);
    }

    bool SelectorMedia::Launch(unsigned index)
    {
        return aurora_selector::ExperimentalLaunch(seed, transport, aurora_selector::Images[index],
                                                   staged);
    }
} // namespace

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
    next_button.Init(seed.GetPin(10), 1000.f);     // Reverse
    load_button.Init(seed.GetPin(1), 1000.f);      // Freeze
    category_button.Init(seed.GetPin(13), 1000.f); // Shift changes category

    daisy::I2CHandle i2c;
    daisy::I2CHandle::Config led_config;
    led_config.periph = daisy::I2CHandle::Config::Peripheral::I2C_1;
    led_config.pin_config = {seed.GetPin(11), seed.GetPin(12)};
    led_config.speed = daisy::I2CHandle::Config::Speed::I2C_1MHZ;
    led_config.mode = daisy::I2CHandle::Config::Mode::I2C_MASTER;
    i2c.Init(led_config);
    leds.Init(i2c, {SELECTOR_LED_DRIVER_A, SELECTOR_LED_DRIVER_B}, aurora::led_dma_buffer_a,
              aurora::led_dma_buffer_b);
    for (int channel = 0; channel < 32; ++channel)
    {
        leds.SetLed(channel, 0.f);
    }

    media_initialization = transport.Initialize();

    std::uint32_t last_control = daisy::System::GetNow();
    const std::uint32_t buttons_started = last_control;
    while (true)
    {
        if (media_initialization != daisy_development::MediaInitialization::Ready)
        {
            // Latched until reset; neither readiness nor a button can hide it
            // or authorize loading/launch. Do not pump failed USB handles.
            controller.FailInitialization();
            SetStatus(0.4f, 0.f, 0.f);
            daisy::System::Delay(20);
            continue;
        }
        transport.Pump();
        const auto now = daisy::System::GetNow();
        if (now == last_control)
        {
            continue;
        }
        last_control = now;
        next_button.Debounce();
        load_button.Debounce();
        category_button.Debounce();
        if (category_button.RawState() && next_button.RawState())
        {
            controller.RequestMenu();
        }
        // Allow a stable held combination before the first possible autoboot.
        if (now - buttons_started < 30U)
        {
            continue;
        }
        const bool category_changed = category_button.FallingEdge();
        const bool selection_changed = next_button.FallingEdge();
        controller.Update(category_changed, selection_changed, load_button.FallingEdge(),
                          []
                          {
                              SetStatus(0.4f, 0.2f, 0.f);
                          });
        if (now % 20U == 0U)
        {
            switch (controller.GetState())
            {
            case State::Waiting:
                SetStatus(0.f, 0.f, 0.1f);
                break;
            case State::Empty:
                SetStatus(0.4f, 0.f, 0.f);
                break;
            case State::Selected:
                SetStatus(0.15f, 0.15f, 0.15f);
                break;
            case State::Loading:
                SetStatus(0.4f, 0.2f, 0.f);
                break;
            case State::Error:
                SetStatus(0.4f, 0.f, 0.f);
                break;
            }
        }
    }
}
