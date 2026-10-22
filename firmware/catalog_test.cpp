// Host-only presentation/size invariants, not payload or hardware execution.
#include "images.hpp"
#include <cassert>
#include <cstring>

int main() {
    using namespace aurora_selector;
    constexpr auto count = sizeof(Images) / sizeof(Images[0]);
    static_assert(count >= 14);
    static_assert(StagingCapacity() >= 182240);
    constexpr std::array<unsigned, 14> groups{0, 0, 1, 1, 2, 1, 2, 1, 1, 2, 2, 2, 0, 0};
    for (unsigned i = 0; i < groups.size(); ++i) {
        assert(Categories[i] == groups[i]);
    }
    // Six arcs must represent every available entry even with old versions present.
    for (unsigned category = 0; category < CategoryColors.size(); ++category) {
        unsigned members = 0;
        for (const auto group : Categories) {
            members += group == category;
        }
        assert(members <= 6);
    }
    assert(Images[0].menu_color == (std::array<float, 3>{0.f, 0.f, .4f}));
    assert(Images[1].menu_color == (std::array<float, 3>{0.f, .4f, 0.f}));
    // Freeze the public mapping independently of the shared constants. Versions
    // share a family color; unrelated families must remain distinguishable.
    constexpr unsigned families[] = {0, 1, 2, 3, 4, 5, 6, 5, 5, 6, 7, 8, 9, 10};
    constexpr std::array<float, 3> colors[] = {
        {0.f, 0.f, .4f}, {0.f, .4f, 0.f}, {0.f, .4f, .4f}, {.4f, 0.f, .4f},
        {.4f, .2f, 0.f}, {.4f, .4f, 0.f}, {.4f, .4f, .4f}, {.4f, 0.f, 0.f},
        {0.f, .2f, .4f}, {.4f, 0.f, .1f}, {.4f, .4f, 0.f},
    };
    for (unsigned i = 0; i < count; ++i) {
        if (i < 14) {
            assert(Images[i].menu_color == colors[families[i]]);
        }
        assert(std::strlen(Images[i].sha256) == 64);
        assert(Images[i].size <= StagingCapacity());
        for (unsigned j = 0; j < i; ++j) {
            if (FirmwareCatalog[i].family == FirmwareCatalog[j].family) {
                assert(Images[i].menu_color == Images[j].menu_color);
            } else if (Categories[i] == Categories[j]) {
                // Compare RGB proportions so brightness alone cannot distinguish
                // families. This is a palette guard, not a perceptual LED model.
                const auto& a = Images[i].menu_color;
                const auto& b = Images[j].menu_color;
                const float sum_a = a[0] + a[1] + a[2];
                const float sum_b = b[0] + b[1] + b[2];
                assert(sum_a > 0.f && sum_b > 0.f);
                float distance_squared = 0.f;
                for (unsigned channel = 0; channel < 3; ++channel) {
                    const float delta = a[channel] / sum_a - b[channel] / sum_b;
                    distance_squared += delta * delta;
                }
                // At least 0.4 apart in normalized RGB: rejects the old
                // green/lime and white/pale-red pairs, including dimmer copies.
                assert(distance_squared >= .16f);
            }
            assert(std::strcmp(Images[i].filename, Images[j].filename) != 0);
            assert(std::strcmp(Images[i].sha256, Images[j].sha256) != 0);
        }
    }
    // Same-family versions are distinct byte contracts, not filename aliases.
    assert(Images[7].size == 91200 && Images[8].size == 92208);
    assert(Images[9].size == 86952);
    assert(Images[7].vectors.reset == 0x24001675U);
    assert(Images[9].vectors.reset == 0x24000f4dU);
    assert(Images[11].size == 151420);
    assert(Images[11].vectors.reset == 0x24000a49U);
}
