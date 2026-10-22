#pragma once
#include "images.hpp"
#include "../support/supported_image_menu.hpp"

namespace aurora_selector {
// Read installed slots once, then handle menu presses and launch requests.
// An explicit menu action cancels the one automatic boot attempt.
template <class Firmware> class SelectorController {
  public:
    enum class State { Empty, Selected, Loading, Error };
    explicit SelectorController(Firmware& firmware) : firmware_(firmware), menu_(Categories) {}
    void RequestMenu() { boot_pending_ = false; }
    State GetState() const { return state_; }
    unsigned Selected() const { return static_cast<unsigned>(menu_.Selected()); }
    const auto& Menu() const { return menu_; }

    template <class Loading>
    void Update(bool category_changed, bool selection_changed, bool load_pressed, Loading loading) {
        if (category_changed || selection_changed)
            RequestMenu();
        bool automatic = false;
        if (!started_) {
            started_ = true;
            state_ = State::Loading;
            loading();
            std::array<bool, ImageCount> installed{};
            if (!firmware_.Open(installed)) {
                state_ = State::Error;
                return;
            }
            menu_.SetInstalled(installed);
            if (boot_pending_) {
                boot_pending_ = false;
                const auto saved = firmware_.BootChoice();
                automatic = saved >= 0 && menu_.Select(saved);
            }
            state_ = menu_.HasSelection() ? State::Selected : State::Empty;
        }
        // Empty and unavailable storage both have no selectable image. Keep
        // their existing Empty/Error state until reset.
        if (!menu_.HasSelection())
            return;
        // Navigation takes precedence over Freeze in the same update.
        if (category_changed || selection_changed) {
            if (category_changed)
                menu_.NextCategory();
            else
                menu_.Next();
            state_ = State::Selected;
            return;
        }
        if (!load_pressed && !automatic)
            return;

        state_ = State::Loading;
        loading();
        const auto selected = Selected();
        if (!firmware_.Authenticate(selected)) {
            state_ = State::Error;
            return;
        }
        // Running now and remembering the next boot are separate actions.
        // Launch still performs the full staged-byte and hardware checks.
        if (!automatic && !firmware_.Remember(selected))
            firmware_.SaveWarning();
        if (!firmware_.Launch(selected))
            state_ = State::Error;
    }

  private:
    Firmware& firmware_;
    daisy_development::SupportedImageMenu<ImageCount> menu_;
    State state_ = State::Empty;
    bool started_ = false;
    bool boot_pending_ = true;
};
} // namespace aurora_selector
