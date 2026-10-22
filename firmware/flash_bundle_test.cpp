#include "../support/flash_bundle.hpp"
#include <cassert>
#include <vector>
using namespace aurora_selector;
void word(std::uint8_t* p, std::uint32_t v) {
    for (unsigned i = 0; i < 4; ++i)
        p[i] = v >> (8 * i);
}
int main() {
    std::vector<std::uint8_t> flash(BundleEnd, 255);
    assert(!InstalledImage(flash.data(), flash.size(), 0));
    auto* slot = flash.data() + BundleOffsets[0];
    word(slot, Images[0].vectors.stack);
    word(slot + 4, Images[0].vectors.reset);
    assert(InstalledImage(flash.data(), flash.size(), 0) == slot);
    assert(!InstalledImage(flash.data(), flash.size(), 1));
    assert(!InstalledImage(flash.data(), flash.size(), ImageCount));
    assert(!InstalledImage(flash.data(), BundleEnd - 1, 0));
    assert(!InstalledImage(nullptr, BundleEnd, 0));
    word(slot + 4, 0);
    assert(!InstalledImage(flash.data(), flash.size(), 0));
}
