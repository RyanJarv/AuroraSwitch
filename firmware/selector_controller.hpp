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

        void FailInitialization()
        {
            menu_.Clear();
            state_ = State::Error;
        }

        template<class Loading> void Update(bool category_changed,
                                            bool selection_changed,
                                            bool load_pressed, Loading loading)
        {
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
                selected_ = static_cast<unsigned>(menu_.Selected());
                state_ = !media_.Ready() ? State::Waiting
                    : menu_.HasSelection() ? State::Selected : State::Empty;
            }
            media_was_ready_ = media_.Ready();
            if(category_changed || selection_changed)
            {
                if(category_changed) menu_.NextCategory();
                else menu_.Next();
                selected_ = static_cast<unsigned>(menu_.Selected());
                state_ = !media_.Ready() ? State::Waiting
                    : menu_.HasSelection() ? State::Selected : State::Empty;
            }
            if(load_pressed && !category_changed && !selection_changed
               && media_.Ready() && menu_.HasSelection())
            {
                state_ = State::Loading;
                loading();
                if(!media_.Authenticate(selected_) || !media_.Ready()
                   || !media_.Launch(selected_))
                    state_ = State::Error;
            }
        }

        State GetState() const { return state_; }
        unsigned Selected() const { return selected_; }
        const auto& Menu() const { return menu_; }

      private:
        Media& media_;
        daisy_development::SupportedImageMenu<sizeof(Images) / sizeof(Images[0])>
            menu_;
        unsigned selected_ = 0;
        State state_ = State::Waiting;
        bool media_was_ready_ = false;
    };
}
