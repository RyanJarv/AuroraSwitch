// Host-only presentation/size invariants, not payload or hardware execution.
#include "images.hpp"
#include <cassert>
#include <cstring>

int main() {
    using namespace aurora_selector;
    // Six arcs must represent every available entry even with old versions present.
    for (unsigned category = 0; category < CategoryColors.size(); ++category) {
        unsigned members = 0;
        for (const auto group : Categories) {
            members += group == category;
        }
        assert(members <= 6);
    }
    // Exact historical identities, colors and positions live in the independent
    // catalog_launch_v1.json regression. Check catalog-wide invariants here.
    for (unsigned i = 0; i < ImageCount; ++i) {
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
                // near-green and white/pale-red pairs, including dimmer copies.
                assert(distance_squared >= .16f);
            }
            assert(std::strcmp(Images[i].filename, Images[j].filename) != 0);
            assert(std::strcmp(Images[i].sha256, Images[j].sha256) != 0);
        }
    }
}
