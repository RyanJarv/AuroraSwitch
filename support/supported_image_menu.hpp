#pragma once

// Availability and selection only; launch approval belongs to the selector.
#include <array>
#include <cstddef>

namespace daisy_development {
// Menu presence never authorizes execution; launch verifies the exact image.
template <std::size_t Count> class SupportedImageMenu {
    static_assert(Count > 0, "A supported catalog must not be empty");

  public:
    explicit SupportedImageMenu(std::array<unsigned, Count> categories) : categories_(categories) {}
    // Presence comes from catalog files on USB. Payload bytes
    // are authenticated separately, immediately before a requested launch.
    void SetInstalled(const std::array<bool, Count>& installed) {
        available_ = installed;
        if (!available_[selected_])
            for (std::size_t index = 0; index < Count; ++index)
                if (available_[index]) {
                    selected_ = index;
                    break;
                }
    }

    bool HasSelection() const { return available_[selected_]; }
    std::size_t Selected() const { return selected_; }
    bool Select(std::size_t index) {
        if (index >= Count || !available_[index])
            return false;
        selected_ = index;
        return true;
    }
    void Next() {
        if (!HasSelection())
            return;
        for (std::size_t distance = 1; distance <= Count; ++distance) {
            const auto index = (selected_ + distance) % Count;
            if (available_[index] && categories_[index] == Category()) {
                selected_ = index;
                return;
            }
        }
    }

    unsigned Category() const { return categories_[selected_]; }
    // Skip empty categories, preserving catalog order within each group.
    void NextCategory() {
        if (!HasSelection())
            return;
        const unsigned current = Category();
        std::size_t first = selected_, next = Count;
        for (std::size_t index = 0; index < Count; ++index) {
            if (!available_[index])
                continue;
            const auto category = categories_[index];
            if (category < categories_[first])
                first = index;
            if (category > current && (next == Count || category < categories_[next]))
                next = index;
        }
        // Advance to the next nonempty category, or wrap to the first one.
        selected_ = next == Count ? first : next;
    }
    std::size_t Position() const {
        std::size_t position = 0;
        for (std::size_t index = 0; index < selected_; ++index)
            if (available_[index] && categories_[index] == Category())
                ++position;
        return position;
    }

  private:
    std::array<bool, Count> available_{};
    std::array<unsigned, Count> categories_{};
    std::size_t selected_ = 0;
};
} // namespace daisy_development
