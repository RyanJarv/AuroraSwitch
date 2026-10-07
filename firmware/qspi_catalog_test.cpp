// Opt-in admission must add only the reviewed QSPI image, never reinterpret SRAM.
#include "images.hpp"
#include <cassert>
#include <cstring>
int main()
{
    using namespace aurora_selector;
    static_assert(sizeof(Images) / sizeof(Images[0]) == 14);
    static_assert(StagingCapacity() == 182240U);
    assert(Images[13].execution == Execution::Sram);
    for(unsigned index : {12U})
    {
        auto copied = Images[index];
        assert(IsReviewedQspiImage(copied));
        copied.size += 1;
        assert(!IsReviewedQspiImage(copied));
        copied = Images[index];
        copied.execution = Execution::Sram;
        assert(!IsReviewedQspiImage(copied));
        copied = Images[index];
        copied.path = "0:/aurora/unknown.bin";
        assert(!IsReviewedQspiImage(copied));
    }
    assert(!IsReviewedQspiImage(Images[13]));
    // Retired QSPI Fata bytes must not regain admission through a copied entry.
    auto retired = Images[11];
    retired.path = "0:/aurora/FataMorgana-QSPI.bin";
    retired.sha256 = "d97311056ac1562b09e0afb518aeb3587d3df9bbb4a031d68da22be52bee8da1";
    retired.vectors.reset = 0x90040a49U;
    retired.menu_color = {0.f, 0.3f, 0.1f};
    retired.execution = Execution::Qspi;
    assert(!IsReviewedQspiImage(retired));
    assert(Images[11].execution == Execution::Sram);
    assert(std::strcmp(Images[11].path, "0:/aurora/FataMorgana.bin") == 0);
    for(unsigned i = 0; i < 14; ++i)
        for(unsigned j = 0; j < i; ++j)
        {
            // Main keeps older Flux/Morse versions in their family color.
            if(i >= 12 || j >= 12)
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
