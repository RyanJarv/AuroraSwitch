#include "../support/resident_image.hpp"
#include <cassert>
#include <vector>
using namespace aurora_selector;
void word(std::uint8_t* bytes, std::uint32_t value)
{ for(unsigned i = 0; i < 4; ++i) bytes[i] = value >> (8 * i); }
int main()
{
    std::vector<std::uint8_t> mapped(0x91000, 0xff), original(DirtImageSize, 0x35);
    assert(!ResidentPresent(mapped.data()));
    std::copy(original.begin(), original.end(), mapped.begin());
    word(mapped.data() + 4, SELECTOR_RESIDENT_ENTRY);
    assert(ResidentPresent(mapped.data()));
    assert(ResidentDirtMatches(mapped.data(), original.data(), original.size()));
    // Only the reset word may differ, never stack, vectors, code or last byte.
    for(const auto offset : {0U, 4U, 8U, 0x958U, DirtImageSize - 1U})
    {
        mapped[offset] ^= 1;
        assert(!ResidentDirtMatches(mapped.data(), original.data(), original.size()));
        mapped[offset] ^= 1;
    }
    const std::uint8_t boot[]{1, 2, 3, 4};
    const auto boot_offset = (SELECTOR_RESIDENT_ENTRY & ~1U) - SELECTOR_RESIDENT_BASE;
    std::memcpy(mapped.data() + boot_offset, boot, sizeof(boot));
    auto* metadata = mapped.data() + SELECTOR_RESIDENT_METADATA - SELECTOR_RESIDENT_BASE;
    auto* image = mapped.data() + SELECTOR_RESIDENT_IMAGE - SELECTOR_RESIDENT_BASE;
    word(image, 0x20020000U);
    word(image + 4, 0x24000009U);
    word(metadata, 0x31525341U);
    word(metadata + 4, 32);
    word(metadata + 8, ~32U);
    word(metadata + 12, ResidentChecksum(image, 32));
    assert(ResidentBootMatches(mapped.data(), boot, sizeof(boot)));
    for(const auto offset : {boot_offset, SELECTOR_RESIDENT_METADATA - SELECTOR_RESIDENT_BASE,
            SELECTOR_RESIDENT_METADATA - SELECTOR_RESIDENT_BASE + 4U,
            SELECTOR_RESIDENT_METADATA - SELECTOR_RESIDENT_BASE + 8U,
            SELECTOR_RESIDENT_METADATA - SELECTOR_RESIDENT_BASE + 12U,
            SELECTOR_RESIDENT_IMAGE - SELECTOR_RESIDENT_BASE + 31U})
    {
        mapped[offset] ^= 1;
        assert(!ResidentBootMatches(mapped.data(), boot, sizeof(boot)));
        mapped[offset] ^= 1;
    }
}
