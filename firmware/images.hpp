#pragma once

// Shared launch catalog for firmware, host authentication, and payload preparation.
#include "../support/sram_image_vectors.hpp"
#include <array>

namespace aurora_selector
{
    // Exact file contract and its panel color, not a general firmware description.
    struct Image
    {
        const char* path;
        const char* sha256;
        std::size_t size;
        daisy_development::SramImageVectors vectors;
        std::array<float, 3> menu_color;
    };

    // Local byte identities, not signed vendor/source provenance.
    constexpr Image Images[] = {
        {"0:/aurora/AR_FDN_v1_2_2.bin",
         "13e774782ce3367675bcee895900f63915d414127d1c2d797e254e020c55f916",
         104196U, {0x20020000U, 0x24000959U}, {0.f, 0.f, 0.4f}},
        {"0:/aurora/Aurora_v1_4_4.bin",
         "0ec2dc67748c6864edad7b7910bf65c26d2db4f6c38034f07cfcf286615882e4",
         181868U, {0x20020000U, 0x2400070dU}, {0.f, 0.4f, 0.f}},
        // Community byte identities. Runtime/recovery checks are separate from
        // this exact admission predicate; these are not signed author releases.
        {"0:/aurora/AuroraEchoGarden_v0_3_1_STABLE.bin",
         "1ecb8e3efff6e5b5360cb6d6beb87455d4cd0bf58a1a26ca07d670603ebe8442",
         94704U, {0x20020000U, 0x240017d1U}, {0.f, 0.4f, 0.4f}},
        {"0:/aurora/AuroraCloudscapeX.bin",
         "564c6791142e112e2098932a185fe112999e3acf06745c1d1e5a9916a09ad8ea",
         98968U, {0x20020000U, 0x24001959U}, {0.4f, 0.f, 0.4f}},
        {"0:/aurora/TheOscillatorIsALie_v0_0_2.bin",
         "5279d277c61d3d3693b94171bda5d48124e7529920f4ef422f7ff2812bb5c600",
         103604U, {0x20020000U, 0x24000959U}, {0.4f, 0.2f, 0.f}},
        {"0:/aurora/flux-capacitor-0.3.0.bin",
         "383f0fbdca991d939c4b35cd1e2f68504d573c2416e2c798b8a699ea3cf990f3",
         92936U, {0x20020000U, 0x24001675U}, {0.4f, 0.4f, 0.f}},
        {"0:/aurora/aurora-morse-0.2.0.bin",
         "001ac1ffd668fc29f5a936b502f5235e196671caf4f21924f04aa71d99d4d9d1",
         88264U, {0x20020000U, 0x24000f4dU}, {0.4f, 0.4f, 0.4f}},
    };

    // Size the shared buffer for the largest entry, rounded for DMA/cache alignment.
    constexpr std::size_t StagingCapacity()
    {
        std::size_t maximum = 0;
        for(const auto& image : Images)
            if(image.size > maximum)
                maximum = image.size;
        return (maximum + 31U) & ~std::size_t(31U);
    }
    constexpr std::uintptr_t StagingAddress = 0x30008000U;
    constexpr std::uintptr_t StagingLimit = 0x30040000U;
    static_assert(StagingCapacity() <= StagingLimit - StagingAddress,
                  "Reviewed image set exceeds internal staging SRAM");

    // Match staged bytes and their caller-computed digest to one reviewed entry.
    inline bool VerifyImage(const std::uint8_t* bytes, std::size_t size,
                            const Image& image, const std::uint8_t* digest)
    {
        daisy_development::SramImageVectors vectors{};
        if(digest == nullptr || size != image.size
           || !daisy_development::ReadSramImageVectors(
               bytes, size, 480U * 1024U, vectors)
           || vectors.stack != image.vectors.stack
           || vectors.reset != image.vectors.reset)
            return false;
        constexpr char hex[] = "0123456789abcdef";
        for(std::size_t i = 0; i < 32U; ++i)
        {
            if(hex[digest[i] >> 4] != image.sha256[i * 2U]
               || hex[digest[i] & 15U] != image.sha256[i * 2U + 1U])
                return false;
        }
        return image.sha256[64] == '\0';
    }
}
