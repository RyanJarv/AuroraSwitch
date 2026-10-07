// Opt-in admission must add only the reviewed QSPI image, never reinterpret SRAM.
#include "images.hpp"
#include <cassert>
#include <cstring>
int main()
{
    using namespace aurora_selector;
    static_assert(sizeof(Images) / sizeof(Images[0]) == 15);
    static_assert(StagingCapacity() == 182240U);
    assert(Images[13].execution == Execution::Qspi);
    assert(Images[13].vectors.reset == 0x90040a49U);
    assert(Images[14].execution == Execution::Sram);
    for(unsigned i = 0; i < 15; ++i)
        for(unsigned j = 0; j < i; ++j)
        {
            assert(Images[i].menu_color != Images[j].menu_color);
            assert(std::strcmp(Images[i].path, Images[j].path) != 0);
        }
    assert(Images[12].size == 95196U);
    assert(Images[12].vectors.reset == 0x90040959U);
    assert(Images[12].execution == Execution::Qspi);
    for(unsigned i = 0; i < 12; ++i)
        assert(Images[i].execution == Execution::Sram);
    // Cross-target vectors fail even if the caller supplies the right digest.
    std::uint8_t bytes[95196]{};
    const std::uint32_t vectors[] = {0x20020000U, 0x90040959U};
    std::memcpy(bytes, vectors, sizeof(vectors));
    std::uint8_t digest[32]{};
    for(unsigned i = 0; i < 32; ++i)
    {
        const auto hex = [](char c) { return c <= '9' ? c - '0' : c - 'a' + 10; };
        digest[i] = (hex(Images[12].sha256[2*i]) << 4) | hex(Images[12].sha256[2*i+1]);
    }
    assert(VerifyImage(bytes, sizeof(bytes), Images[12], digest));
    auto wrong = Images[12];
    wrong.execution = Execution::Sram;
    assert(!VerifyImage(bytes, sizeof(bytes), wrong, digest));
    bytes[4] ^= 1;
    assert(!VerifyImage(bytes, sizeof(bytes), Images[12], digest));
}
