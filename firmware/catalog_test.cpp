// Host-only presentation/size invariants, not payload or hardware execution.
#include "images.hpp"
#include <cassert>
#include <cstring>

int main()
{
    using namespace aurora_selector;
    static_assert(sizeof(Images) / sizeof(Images[0]) == 5);
    static_assert(StagingCapacity() == 181888);
    assert(Images[0].menu_color == (std::array<float, 3>{0.f, 0.f, .4f}));
    assert(Images[1].menu_color == (std::array<float, 3>{0.f, .4f, 0.f}));
    for(unsigned i = 0; i < 5; ++i)
    {
        assert(std::strlen(Images[i].sha256) == 64);
        assert(Images[i].size <= StagingCapacity());
        for(unsigned j = 0; j < i; ++j)
        {
            assert(Images[i].menu_color != Images[j].menu_color);
            assert(std::strcmp(Images[i].path, Images[j].path) != 0);
        }
    }
}
