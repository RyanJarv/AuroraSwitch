#pragma once

// Shared launch catalog for firmware, host authentication, and payload preparation.
#include "../support/sram_image_vectors.hpp"
#include <array>
#include <cstring>

namespace aurora_selector
{
    enum class Execution { Sram, Qspi };
    // Stable family colors: every version reuses its family's entry here.
    // Do not derive colors from discovery order or change them for new versions.
    namespace menu_colors
    {
        constexpr std::array<float, 3> Fdn{0.f, 0.f, 0.4f};
        constexpr std::array<float, 3> Aurora{0.f, 0.4f, 0.f};
        constexpr std::array<float, 3> EchoGarden{0.f, 0.4f, 0.4f};
        constexpr std::array<float, 3> Cloudscape{0.4f, 0.f, 0.4f};
        constexpr std::array<float, 3> Oscillator{0.4f, 0.2f, 0.f};
        constexpr std::array<float, 3> Flux{0.4f, 0.4f, 0.f};
        constexpr std::array<float, 3> Morse{0.4f, 0.4f, 0.4f};
        constexpr std::array<float, 3> Tempest{0.4f, 0.2f, 0.2f};
        constexpr std::array<float, 3> FataMorgana{0.f, 0.2f, 0.4f};
        constexpr std::array<float, 3> DirtVerb{0.4f, 0.f, 0.1f};
        constexpr std::array<float, 3> HpFilter{0.1f, 0.4f, 0.f};
    }

    // Exact file contract and its panel color, not a general firmware description.
    struct Image
    {
        const char* path;
        const char* sha256;
        std::size_t size;
        daisy_development::SramImageVectors vectors;
        std::array<float, 3> menu_color;
        Execution execution = Execution::Sram;
    };

    // Local byte identities, not signed vendor/source provenance.
    constexpr Image Images[] = {
        {"0:/aurora/AR_FDN_v1_2_2.bin",
         "13e774782ce3367675bcee895900f63915d414127d1c2d797e254e020c55f916",
         104196U, {0x20020000U, 0x24000959U}, menu_colors::Fdn},
        {"0:/aurora/Aurora_v1_4_4.bin",
         "0ec2dc67748c6864edad7b7910bf65c26d2db4f6c38034f07cfcf286615882e4",
         181868U, {0x20020000U, 0x2400070dU}, menu_colors::Aurora},
        // Community byte identities. Runtime/recovery checks are separate from
        // this exact admission predicate; these are not signed author releases.
        {"0:/aurora/AuroraEchoGarden_v0_3_1_STABLE.bin",
         "1ecb8e3efff6e5b5360cb6d6beb87455d4cd0bf58a1a26ca07d670603ebe8442",
         94704U, {0x20020000U, 0x240017d1U}, menu_colors::EchoGarden},
        {"0:/aurora/AuroraCloudscapeX.bin",
         "564c6791142e112e2098932a185fe112999e3acf06745c1d1e5a9916a09ad8ea",
         98968U, {0x20020000U, 0x24001959U}, menu_colors::Cloudscape},
        {"0:/aurora/TheOscillatorIsALie_v0_0_2.bin",
         "5279d277c61d3d3693b94171bda5d48124e7529920f4ef422f7ff2812bb5c600",
         103604U, {0x20020000U, 0x24000959U}, menu_colors::Oscillator},
        {"0:/aurora/flux-capacitor-0.3.0.bin",
         "383f0fbdca991d939c4b35cd1e2f68504d573c2416e2c798b8a699ea3cf990f3",
         92936U, {0x20020000U, 0x24001675U}, menu_colors::Flux},
        {"0:/aurora/aurora-morse-0.2.0.bin",
         "001ac1ffd668fc29f5a936b502f5235e196671caf4f21924f04aa71d99d4d9d1",
         88264U, {0x20020000U, 0x24000f4dU}, menu_colors::Morse},
        // Version-specific entries keep existing payload identities and indices.
        // Older-version virtual checks pass; physical testing remains open.
        {"0:/aurora/flux-capacitor-0.1.0.bin",
         "23435b32ffe5f8d715fc459da9590b3b71bf44fbf28266aeddebb555b23b9a97",
         91200U, {0x20020000U, 0x24001675U}, menu_colors::Flux},
        {"0:/aurora/flux-capacitor-0.2.0.bin",
         "4ea0ab917fad8bfa225e6c95551f51f6c0817c31f7b3c82e65443e0c5e189239",
         92208U, {0x20020000U, 0x24001675U}, menu_colors::Flux},
        {"0:/aurora/aurora-morse-0.1.0.bin",
         "f514628388a9859334b4d9348d4e55964d46c19472152770c6afdc04bbf7269b",
         86952U, {0x20020000U, 0x24000f4dU}, menu_colors::Morse},
        // Tempest's virtual handoff/audio checks pass; physical checks remain open.
        // Its own settings initialization may write QSPI sector 0x2000.
        {"0:/aurora/Tempest_v1_0_0.bin",
         "6fdb962135b2784813523a70e73bd1644b1d5b1e6637bb042c714dd7830931eb",
         109872U, {0x20020000U, 0x240033f5U}, menu_colors::Tempest},
        // Exact experimental RAM build, not the author's QSPI configuration.
        // Source manifest dcaecaf4...150f7; USB and handoff checks remain open.
        {"0:/aurora/FataMorgana.bin",
         "35bc0bebafa7736ccc61ca788e5f3c3aa2fe4168534761722768eb907743d005",
         151420U, {0x20020000U, 0x24000a49U}, menu_colors::FataMorgana},
#ifdef SELECTOR_QSPI_HANDOFF
        // Experimental exact-image flash path; absent from default RAM builds.
        {"0:/aurora/DirtVerb 1.1.bin",
         "e1775fb6c46dd83e33abaf599eb6d6089b7ff56692b42ac48748d2d1555d2784",
         95196U, {0x20020000U, 0x90040959U}, menu_colors::DirtVerb, Execution::Qspi},
        // Supplied exact HP-filter variant fits the reviewed staging region.
        {"0:/aurora/Aurora_v1-4-6_hpfilt.bin",
         "94f4200efdf47cfb0c055fa51896da4d8c8d0f8b6a6d0a9bc9ec31553d85ebc7",
         182212U, {0x20020000U, 0x2400070dU}, menu_colors::HpFilter},
#endif
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

#ifdef SELECTOR_QSPI_HANDOFF
    // Header-local catalogs have different addresses in each translation unit.
    // Match the reviewed value, never a caller's pointer identity.
    inline bool IsReviewedQspiImage(const Image& image)
    {
        for(const auto& entry : Images)
            if(entry.execution == Execution::Qspi && image.execution == entry.execution
               && image.size == entry.size && image.vectors.stack == entry.vectors.stack
               && image.vectors.reset == entry.vectors.reset && image.menu_color == entry.menu_color
               && image.path && image.sha256
               && std::strcmp(image.path, entry.path) == 0
               && std::strcmp(image.sha256, entry.sha256) == 0)
                return true;
        return false;
    }
#endif

    // Match staged bytes and their caller-computed digest to one reviewed entry.
    inline bool VerifyImage(const std::uint8_t* bytes, std::size_t size,
                            const Image& image, const std::uint8_t* digest)
    {
        daisy_development::SramImageVectors vectors{};
        if(digest == nullptr || size != image.size
           || (image.execution != Execution::Sram && image.execution != Execution::Qspi)
           || !daisy_development::ReadImageVectors(
               bytes, size, 480U * 1024U,
               image.execution == Execution::Qspi ? 0x90040000U : 0x24000000U, vectors)
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
