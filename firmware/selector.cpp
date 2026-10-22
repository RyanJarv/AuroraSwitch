// Panel menu and read-only loading; handoff.cpp owns irreversible launch.
#include "daisy_seed.h"
#include "images.hpp"
#include "selector_controller.hpp"
#include "../support/choice_journal.hpp"
#include "../support/flash_bundle.hpp"
#include "flash_driver.hpp"
#include "mbedtls/sha256.h"
#include "handoff.hpp"

namespace {
daisy::DaisySeed seed;
daisy::Switch next_button, load_button;
daisy::Switch category_button;
aurora_selector::ChoiceFlashDriver flash_driver{seed.qspi};
using LedDriver = daisy::LedDriverPca9685<2, true>;
LedDriver leds;
DMA_BUFFER_MEM_SECTION LedDriver::DmaBuffer led_dma_a, led_dma_b;
__attribute__((section(".selector_staging"),
               aligned(32))) std::uint8_t staged[aurora_selector::StagingCapacity()];
struct InstalledFirmware {
    bool Open(std::array<bool, aurora_selector::ImageCount>& installed);
    bool Authenticate(unsigned index);
    bool Launch(unsigned index);
    int BootChoice();
    bool Remember(unsigned index);
    void SaveWarning();
};
InstalledFirmware firmware;
aurora_selector::SelectorController<InstalledFirmware> controller(firmware);
using aurora_selector::Color;
using aurora_selector::GetColor;
using State = aurora_selector::SelectorController<InstalledFirmware>::State;
// Freeze shows load status; Reverse shows the current catalog selection.
void SetStatus(const std::array<float, 3>& status) {
    // SDK LED_FREEZE wiring: channels 3, 4, 5.
    for (unsigned component = 0; component < status.size(); ++component)
        leds.SetLed(3 + component, status[component]);
    // Selection colors belong to the exact catalog, not two special indices.
    const auto& menu = controller.Menu();
    const bool has_selection = menu.HasSelection();
    const auto position = menu.Position();
    const auto color = has_selection ? aurora_selector::Images[controller.Selected()].menu_color
                                     : std::array<float, 3>{0.f, 0.f, 0.f};
    for (unsigned component = 0; component < color.size(); ++component)
        leds.SetLed(component, color[component]);
    // SDK arc wiring is non-sequential. One lit arc shows the group's position.
    constexpr unsigned arcs[6][3] = {{6, 7, 8},    {9, 10, 11},  {16, 17, 18},
                                     {12, 13, 14}, {22, 23, 24}, {28, 29, 30}};
    for (unsigned arc = 0; arc < 6; ++arc)
        for (unsigned component = 0; component < 3; ++component)
            leds.SetLed(arcs[arc][component],
                        has_selection && arc == position
                            ? aurora_selector::CategoryColors[menu.Category()][component]
                            : 0.f);
    leds.SwapBuffersAndTransmit();
}

// Stage only the requested image; success authenticates the current RAM copy.
bool InstalledFirmware::Authenticate(unsigned index) {
    // A failed journal operation may leave QSPI unavailable. Do not read
    // a source slot until memory-mapped reads are restored.
    const auto* original = aurora_selector::InstalledImage(
        aurora_selector::MappedFlash(), SELECTOR_QSPI_END - SELECTOR_QSPI_BASE, index);
    if (!original)
        return false;
    const auto& image = aurora_selector::Images[index];
    std::memcpy(staged, original, image.size);
    std::uint8_t digest[32]{};
    return mbedtls_sha256_ret(staged, image.size, digest, 0) == 0 &&
           aurora_selector::VerifyImage(staged, image.size, image, digest);
}

bool InstalledFirmware::Open(std::array<bool, aurora_selector::ImageCount>& installed) {
    const auto* flash = aurora_selector::MappedFlash();
    if (!flash)
        return false;
    for (unsigned i = 0; i < installed.size(); ++i)
        installed[i] = aurora_selector::InstalledImage(
                           flash, SELECTOR_QSPI_END - SELECTOR_QSPI_BASE, i) != nullptr;
    return true;
}

int InstalledFirmware::BootChoice() {
    aurora_selector::ChoiceJournal<aurora_selector::ChoiceFlashDriver> journal{flash_driver};
    return journal.Load().index;
}
bool InstalledFirmware::Remember(unsigned index) {
    aurora_selector::ChoiceJournal<aurora_selector::ChoiceFlashDriver> journal{flash_driver};
    return journal.Save(index);
}
void InstalledFirmware::SaveWarning() {
    // Brief red Freeze LED: this launch will not be remembered reliably.
    // Keep interrupts running until the ordinary checked handoff takes over.
    SetStatus(GetColor(Color::Red).rgb);
    daisy::System::Delay(500);
}
bool InstalledFirmware::Launch(unsigned index) {
    return aurora_selector::LaunchFirmware(seed, index, staged);
}
} // namespace

int main() {
    // Do not use Hardware::Init(): it can persist calibration defaults.
    // No ADC, audio, persistent storage, or low-priority timer is started here.
    // Reuse the pinned System initializer, preserving bootloader clocks/MPU.
    // DaisySeed::Init would reset/reconfigure NOR and initialize unused audio.
    daisy::System::Config system_config;
    system_config.Defaults();
    system_config.skip_clocks = true;
    // Preserve the reviewed cache policy for LED DMA and the final RAM handoff.
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
    leds.Init(i2c, {SELECTOR_LED_DRIVER_A, SELECTOR_LED_DRIVER_B}, led_dma_a, led_dma_b);
    for (int channel = 0; channel < 32; ++channel)
        leds.SetLed(channel, 0.f);

    std::uint32_t last_control = daisy::System::GetNow();
    const std::uint32_t buttons_started = last_control;
    while (true) {
        const auto now = daisy::System::GetNow();
        if (now == last_control)
            continue;
        last_control = now;
        next_button.Debounce();
        load_button.Debounce();
        category_button.Debounce();
        if (category_button.RawState() && next_button.RawState())
            controller.RequestMenu();
        // Allow a stable held combination before the first possible autoboot.
        if (now - buttons_started < 30U)
            continue;
        // Act on presses, so releasing the startup menu combination does not navigate.
        const bool category_changed = category_button.RisingEdge();
        const bool selection_changed = next_button.RisingEdge();
        controller.Update(category_changed, selection_changed, load_button.RisingEdge(),
                          [] { SetStatus(GetColor(Color::Amber).rgb); });
        if (now % 20U == 0U) {
            switch (controller.GetState()) {
            case State::Selected:
                SetStatus({0.15f, 0.15f, 0.15f});
                break;
            case State::Loading:
                SetStatus(GetColor(Color::Amber).rgb);
                break;
            case State::Empty:
            case State::Error:
                SetStatus(GetColor(Color::Red).rgb);
                break;
            }
        }
    }
}
