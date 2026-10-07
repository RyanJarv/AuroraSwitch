// Host-only presentation/size invariants, not payload or hardware execution.
#include "images.hpp"
#include <cassert>
#include <cstring>

int main()
{
    using namespace aurora_selector;
    constexpr auto count = sizeof(Images) / sizeof(Images[0]);
    static_assert(count == 14);
    static_assert(StagingCapacity() == 182240);
    assert(Images[0].menu_color == (std::array<float, 3>{0.f, 0.f, .4f}));
    assert(Images[1].menu_color == (std::array<float, 3>{0.f, .4f, 0.f}));
    // Freeze the public mapping independently of the shared constants. Versions
    // share a family color; unrelated families must remain distinguishable.
    constexpr unsigned families[] = {0, 1, 2, 3, 4, 5, 6, 5, 5, 6, 7, 8, 9, 10};
    constexpr std::array<float, 3> colors[] = {
        {0.f, 0.f, .4f}, {0.f, .4f, 0.f}, {0.f, .4f, .4f},
        {.4f, 0.f, .4f}, {.4f, .2f, 0.f}, {.4f, .4f, 0.f},
        {.4f, .4f, .4f}, {.4f, .2f, .2f}, {0.f, .2f, .4f},
        {.4f, 0.f, .1f}, {.1f, .4f, 0.f},
    };
    for(unsigned i = 0; i < count; ++i)
    {
        assert(Images[i].menu_color == colors[families[i]]);
        assert(std::strlen(Images[i].sha256) == 64);
        assert(Images[i].size <= StagingCapacity());
        for(unsigned j = 0; j < i; ++j)
        {
            assert((Images[i].menu_color == Images[j].menu_color)
                   == (families[i] == families[j]));
            assert(std::strcmp(Images[i].path, Images[j].path) != 0);
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
