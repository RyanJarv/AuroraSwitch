#pragma once

// Selection flow shared by the device entry point and host tests. Media owns
// authentication and launch; this controller never grants lasting approval.
#include "images.hpp"
#include "../support/supported_image_menu.hpp"

namespace aurora_selector
{
    template<class Media> class SelectorController
    {
      public:
        enum class State { Waiting, Empty, Selected, Loading, Error };

        explicit SelectorController(Media& media) : media_(media), menu_(Categories) {}

        void RequestMenu() { boot_pending_ = false; }

        void FailInitialization()
        {
            RequestMenu();
            menu_.Clear();
            state_ = State::Error;
        }

        template<class Loading> void Update(bool category_changed,
                                            bool selection_changed,
                                            bool load_pressed, Loading loading)
        {
            if(category_changed || selection_changed) RequestMenu();
            bool automatic = false;
            const bool media_ready = media_.Ready();
            if(!media_ready)
            {
                menu_.Clear();
                state_ = State::Waiting;
            }
            if(media_ready && !media_was_ready_)
            {
                // Discovery overwrites staging; Freeze authenticates again.
                state_ = State::Loading;
                loading();
                media_.BeginDiscovery();
                menu_.Discover([this](std::size_t index) {
                    return media_.Authenticate(static_cast<unsigned>(index));
                }, [this] { return media_.Ready(); });
                media_.EndDiscovery();
                UpdateSelectionState();
                if(boot_pending_)
                {
                    boot_pending_ = false; // One attempt; reconnect never launches unexpectedly.
                    const int choice = media_.BootChoice();
                    automatic = choice >= 0 && media_.Ready() && menu_.Select(choice);
                }
            }
            media_was_ready_ = media_.Ready();
            if(category_changed || selection_changed)
            {
                if(category_changed) menu_.NextCategory();
                else menu_.Next();
                UpdateSelectionState();
            }
            if((load_pressed || automatic) && !category_changed && !selection_changed
               && media_.Ready() && menu_.HasSelection())
            {
                state_ = State::Loading;
                loading();
                const auto selected = Selected();
                if(!media_.Authenticate(selected) || !media_.Ready()
                   || (!automatic && !media_.Remember(selected)) || !media_.Ready()
                   || !media_.Launch(selected))
                    state_ = State::Error;
            }
        }

        State GetState() const { return state_; }
        unsigned Selected() const { return static_cast<unsigned>(menu_.Selected()); }
        const auto& Menu() const { return menu_; }

      private:
        void UpdateSelectionState()
        {
            if(!media_.Ready())
                state_ = State::Waiting;
            else if(menu_.HasSelection())
                state_ = State::Selected;
            else
                state_ = State::Empty;
        }

        Media& media_;
        daisy_development::SupportedImageMenu<ImageCount> menu_;
        State state_ = State::Waiting;
        bool media_was_ready_ = false;
        bool boot_pending_ = true;
    };
}
