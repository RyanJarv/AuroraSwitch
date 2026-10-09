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
        // Optional small category IDs; the default retains a single flat menu.
        explicit SupportedImageMenu(std::array<unsigned, Count> categories = {})
            : categories_(categories) {}
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
                if(available_[index] && categories_[index] == Category())
                    { selected_ = index; return; }
            }
        }

        unsigned Category() const { return categories_[selected_]; }
        // Skip empty categories, preserving catalog order within each group.
        void NextCategory()
        {
            if(!HasSelection()) return;
            const unsigned current = Category();
            std::size_t choice = selected_;
            unsigned best = ~0U;
            for(std::size_t index = 0; index < Count; ++index)
                if(available_[index] && categories_[index] != current)
                {
                    const unsigned rank = categories_[index] > current
                        ? categories_[index] - current
                        : categories_[index] + Count - current;
                    if(rank < best) { best = rank; choice = index; }
                }
            selected_ = choice;
        }
        std::size_t Position() const
        {
            std::size_t position = 0;
            for(std::size_t index = 0; index < selected_; ++index)
                if(available_[index] && categories_[index] == Category()) ++position;
            return position;
        }

      private:
        std::array<bool, Count> available_{};
        std::array<unsigned, Count> categories_{};
        std::size_t selected_ = 0;
    };
}
