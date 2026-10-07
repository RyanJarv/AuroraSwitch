#pragma once

// Availability and selection only; launch approval belongs to the selector.
#include <array>
#include <cstddef>

namespace daisy_development
{
    // Bounded catalog discovery, not arbitrary binary admission. The caller
    // authenticates each exact catalog entry; discovery never authorizes launch.
    template<std::size_t Count>
    class SupportedImageMenu
    {
        static_assert(Count > 0, "A supported catalog must not be empty");
      public:
        // Invalidate the menu while retaining the last index for a later rescan.
        void Clear() { available_.fill(false); }

        // Preserve a still-valid choice; any media loss discards the entire scan.
        template<class Authenticate, class Ready>
        void Discover(Authenticate authenticate, Ready ready)
        {
            Clear();
            for(std::size_t index = 0; index < Count; ++index)
            {
                if(!ready()) { Clear(); return; }
                available_[index] = authenticate(index);
            }
            if(!ready()) { Clear(); return; }
            if(!available_[selected_])
                for(std::size_t index = 0; index < Count; ++index)
                    if(available_[index]) { selected_ = index; break; }
        }

        bool HasSelection() const { return available_[selected_]; }
        std::size_t Selected() const { return selected_; }
        void Next()
        {
            if(!HasSelection()) return;
            for(std::size_t distance = 1; distance <= Count; ++distance)
            {
                const auto index = (selected_ + distance) % Count;
                if(available_[index]) { selected_ = index; return; }
            }
        }

      private:
        std::array<bool, Count> available_{};
        std::size_t selected_ = 0;
    };
}
