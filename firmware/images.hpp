#pragma once

// One catalog for launch identity, family presentation and host preparation.
// Descriptors grant support only after review; discovery never adds entries.
#include "../support/sram_image_vectors.hpp"
#include <array>
#include <cstring>

namespace aurora_selector {
enum class Execution { Sram, Qspi };
enum class Color { Blue, Green, Cyan, Magenta, Amber, Yellow, White, Red, Azure, RedPink };

struct ColorInfo {
    const char* name;
    const char* css;
    std::array<float, 3> rgb;
};

constexpr ColorInfo Palette[] = {
    {"Blue", "#3787ff", {0.f, 0.f, 0.4f}},   {"Green", "#35bf58", {0.f, 0.4f, 0.f}},
    {"Cyan", "#32c9ce", {0.f, 0.4f, 0.4f}},  {"Magenta", "#d855d5", {0.4f, 0.f, 0.4f}},
    {"Amber", "#dc9a25", {0.4f, 0.2f, 0.f}}, {"Yellow", "#ead342", {0.4f, 0.4f, 0.f}},
    {"White", "#fff", {0.4f, 0.4f, 0.4f}},   {"Red", "#e84444", {0.4f, 0.f, 0.f}},
    {"Azure", "#3ea7dc", {0.f, 0.2f, 0.4f}}, {"Red-pink", "#e84b78", {0.4f, 0.f, 0.1f}},
};

constexpr const ColorInfo& GetColor(Color color) {
    return Palette[static_cast<unsigned>(color)];
}
enum class Category { Reverbs, Delays, Other };

struct CategoryInfo {
    const char* name;
    Color color;
};

constexpr CategoryInfo CategoryDefinitions[] = {
    {"Reverbs", Color::Blue},
    {"Delays", Color::Green},
    {"Synths / Other", Color::Amber},
};
enum class Family {
    Fdn,
    Aurora,
    EchoGarden,
    Cloudscape,
    Oscillator,
    Flux,
    Morse,
    Tempest,
    FataMorgana,
    DirtVerb,
    HpFilter,
    Count
};

struct FamilyInfo {
    const char* id;
    const char* name;
    Category category;
    Color color;
    const char* reference; // Stable Markdown section anchor; versions share it.
};

constexpr FamilyInfo Families[] = {
    {"fdn", "FDN", Category::Reverbs, Color::Blue, "fdn-122--blue"},
    {"aurora", "Aurora", Category::Reverbs, Color::Green, "aurora-144--green"},
    {"echogarden", "EchoGarden", Category::Delays, Color::Cyan, "echogarden-031--cyan"},
    {"cloudscapex", "CloudscapeX", Category::Delays, Color::Magenta, "cloudscapex--magenta"},
    {"oscillator", "The Oscillator Is a Lie", Category::Other, Color::Amber,
     "the-oscillator-is-a-lie-002--amber"},
    {"flux", "Flux Capacitor", Category::Delays, Color::Yellow, "flux-capacitor--yellow"},
    {"morse", "Morse", Category::Other, Color::White, "morse--white"},
    {"tempest", "Tempest", Category::Other, Color::Red, "tempest-100--red"},
    {"fatamorgana", "FataMorgana", Category::Other, Color::Azure,
     "fatamorgana-ram-experiment--azure"},
    {"dirt-verb", "Dirt Verb", Category::Reverbs, Color::RedPink, "dirt-verb-11--red-pink"},
    {"hp-filter", "Aurora HP-filter", Category::Reverbs, Color::Yellow,
     "aurora-hp-filter-variant--yellow"},
};
static_assert(sizeof(Families) / sizeof(Families[0]) == static_cast<unsigned>(Family::Count),
              "Each family needs one metadata record in enum order");

constexpr const FamilyInfo& GetFamily(Family family) {
    return Families[static_cast<unsigned>(family)];
}

// Exact runtime descriptor. Keep metadata out of the handoff ABI.
struct Image {
    const char* path;
    const char* sha256;
    std::size_t size;
    daisy_development::SramImageVectors vectors;
    std::array<float, 3> menu_color{};
    Execution execution = Execution::Sram;
};

struct Firmware {
    Family family;
    const char* id; // Stable image/version ID, independent of menu position.
    const char* name;
    const char* description;
    bool release;
    const char* download; // Optional direct BIN/ZIP URL for the default USB set.
    bool resident;        // Exact reviewed reset-redirection exception, currently Dirt only.
    Image image;
};

// Catalog order is the existing menu/discovery order. Every version keeps
// its own size/hash/vectors; these words come from the authenticated binary.
// Colors and categories are inherited from its family, never duplicated.
constexpr Firmware FirmwareCatalog[] = {
    {Family::Fdn,
     "fdn",
     "FDN 1.2.2",
     "Conventional reverb",
     true,
     "https://www.qubitelectronix.com/s/AR_FDN_v1_2_2.bin",
     false,
     {"0:/aurora/AR_FDN_v1_2_2.bin",
      "13e774782ce3367675bcee895900f63915d414127d1c2d797e254e020c55f916",
      104196U,
      {SELECTOR_DTCM_END, 0x24000959U}}},
    {Family::Aurora,
     "spectral",
     "Aurora 1.4.4",
     "Spectral reverb",
     true,
     "https://www.qubitelectronix.com/s/Aurora_v1_4_4.zip",
     false,
     {"0:/aurora/Aurora_v1_4_4.bin",
      "0ec2dc67748c6864edad7b7910bf65c26d2db4f6c38034f07cfcf286615882e4",
      181868U,
      {SELECTOR_DTCM_END, 0x2400070dU}}},
    {Family::EchoGarden,
     "echogarden-0.3.1",
     "EchoGarden 0.3.1",
     "Multi-line echo and ambience",
     true,
     nullptr,
     false,
     {"0:/aurora/AuroraEchoGarden_v0_3_1_STABLE.bin",
      "1ecb8e3efff6e5b5360cb6d6beb87455d4cd0bf58a1a26ca07d670603ebe8442",
      94704U,
      {SELECTOR_DTCM_END, 0x240017d1U}}},
    {Family::Cloudscape,
     "cloudscapex",
     "CloudscapeX",
     "Modulated delay, diffusion and filter delay",
     true,
     nullptr,
     false,
     {"0:/aurora/AuroraCloudscapeX.bin",
      "564c6791142e112e2098932a185fe112999e3acf06745c1d1e5a9916a09ad8ea",
      98968U,
      {SELECTOR_DTCM_END, 0x24001959U}}},
    {Family::Oscillator,
     "oscillator-0.0.2",
     "The Oscillator Is a Lie 0.0.2",
     "Aliasing/ring-mod oscillator",
     true,
     nullptr,
     false,
     {"0:/aurora/TheOscillatorIsALie_v0_0_2.bin",
      "5279d277c61d3d3693b94171bda5d48124e7529920f4ef422f7ff2812bb5c600",
      103604U,
      {SELECTOR_DTCM_END, 0x24000959U}}},
    {Family::Flux,
     "flux-0.3.0",
     "Flux Capacitor 0.3.0",
     "Tape pitch, delay and coloration",
     true,
     "https://github.com/DaveParr/aurora-flux-capacitor/releases/download/v0.3.0/"
     "flux-capacitor-0.3.0.bin",
     false,
     {"0:/aurora/flux-capacitor-0.3.0.bin",
      "383f0fbdca991d939c4b35cd1e2f68504d573c2416e2c798b8a699ea3cf990f3",
      92936U,
      {SELECTOR_DTCM_END, 0x24001675U}}},
    {Family::Morse,
     "morse-0.2.0",
     "Morse 0.2.0",
     "Clocked rhythmic VCA",
     true,
     "https://github.com/DaveParr/Aurora-Morse/releases/download/v0.2.0/aurora-morse-0.2.0.bin",
     false,
     {"0:/aurora/aurora-morse-0.2.0.bin",
      "001ac1ffd668fc29f5a936b502f5235e196671caf4f21924f04aa71d99d4d9d1",
      88264U,
      {SELECTOR_DTCM_END, 0x24000f4dU}}},
    {Family::Flux,
     "flux-0.1.0",
     "Flux Capacitor 0.1.0",
     "Tape pitch, stop, wow/flutter",
     false,
     nullptr,
     false,
     {"0:/aurora/flux-capacitor-0.1.0.bin",
      "23435b32ffe5f8d715fc459da9590b3b71bf44fbf28266aeddebb555b23b9a97",
      91200U,
      {SELECTOR_DTCM_END, 0x24001675U}}},
    {Family::Flux,
     "flux-0.2.0",
     "Flux Capacitor 0.2.0",
     "Adds tape delay",
     false,
     nullptr,
     false,
     {"0:/aurora/flux-capacitor-0.2.0.bin",
      "4ea0ab917fad8bfa225e6c95551f51f6c0817c31f7b3c82e65443e0c5e189239",
      92208U,
      {SELECTOR_DTCM_END, 0x24001675U}}},
    {Family::Morse,
     "morse-0.1.0",
     "Morse 0.1.0",
     "Three-pattern rhythmic VCA",
     false,
     nullptr,
     false,
     {"0:/aurora/aurora-morse-0.1.0.bin",
      "f514628388a9859334b4d9348d4e55964d46c19472152770c6afdc04bbf7269b",
      86952U,
      {SELECTOR_DTCM_END, 0x24000f4dU}}},
    {Family::Tempest,
     "tempest-1.0.0",
     "Tempest 1.0.0",
     "Multi-bank distortion",
     false,
     nullptr,
     false,
     {"0:/aurora/Tempest_v1_0_0.bin",
      "6fdb962135b2784813523a70e73bd1644b1d5b1e6637bb042c714dd7830931eb",
      109872U,
      {SELECTOR_DTCM_END, 0x240033f5U}}},
    {Family::FataMorgana,
     "fatamorgana-ram",
     "FataMorgana RAM experiment",
     "Dual wavetable synthesizer",
     false,
     nullptr,
     false,
     {"0:/aurora/FataMorgana.bin",
      "35bc0bebafa7736ccc61ca788e5f3c3aa2fe4168534761722768eb907743d005",
      151420U,
      {SELECTOR_DTCM_END, 0x24000a49U}}},
    {Family::DirtVerb,
     "dirt-verb-1.1",
     "Dirt Verb 1.1",
     "Driven reverb with chorus and pitch shifting",
     false,
     nullptr,
     true,
     {"0:/aurora/DirtVerb 1.1.bin",
      "e1775fb6c46dd83e33abaf599eb6d6089b7ff56692b42ac48748d2d1555d2784",
      95196U,
      {SELECTOR_DTCM_END, 0x90040959U},
      {},
      Execution::Qspi}},
    {Family::HpFilter,
     "aurora-hp-filter",
     "Aurora HP-filter variant",
     "Spectral reverb with dry-input high-pass",
     false,
     nullptr,
     false,
     {"0:/aurora/Aurora_v1-4-6_hpfilt.bin",
      "94f4200efdf47cfb0c055fa51896da4d8c8d0f8b6a6d0a9bc9ec31553d85ebc7",
      182212U,
      {SELECTOR_DTCM_END, 0x2400070dU}}},
};
constexpr std::size_t ImageCount = sizeof(FirmwareCatalog) / sizeof(FirmwareCatalog[0]);
constexpr auto Images = [] {
    std::array<Image, ImageCount> images{};
    for (std::size_t i = 0; i < ImageCount; ++i) {
        images[i] = FirmwareCatalog[i].image;
        images[i].menu_color = GetColor(GetFamily(FirmwareCatalog[i].family).color).rgb;
    }
    return images;
}();
constexpr auto Categories = [] {
    std::array<unsigned, ImageCount> categories{};
    for (std::size_t i = 0; i < ImageCount; ++i) {
        categories[i] = static_cast<unsigned>(GetFamily(FirmwareCatalog[i].family).category);
    }
    return categories;
}();
constexpr auto CategoryColors = [] {
    std::array<std::array<float, 3>, sizeof(CategoryDefinitions) / sizeof(CategoryDefinitions[0])>
        colors{};
    for (std::size_t i = 0; i < colors.size(); ++i) {
        colors[i] = GetColor(CategoryDefinitions[i].color).rgb;
    }
    return colors;
}();
constexpr std::size_t ResidentIndex = [] {
    std::size_t index = ImageCount;
    for (std::size_t i = 0; i < ImageCount; ++i) {
        if (FirmwareCatalog[i].resident) {
            if (index != ImageCount) {
                return ImageCount; // More than one is unsupported.
            }
            index = i;
        }
    }
    return index;
}();
static_assert(ResidentIndex < ImageCount, "Exactly one reviewed resident image is required");
constexpr auto ResidentPayload = Images[ResidentIndex];
static_assert(ResidentPayload.execution == Execution::Qspi,
              "Resident payload must execute from QSPI");

// Size the shared buffer for the largest entry, rounded for DMA/cache alignment.
constexpr std::size_t StagingCapacity() {
    std::size_t maximum = 0;
    for (const auto& image : Images) {
        if (image.size > maximum) {
            maximum = image.size;
        }
    }
    return (maximum + 31U) & ~std::size_t(31U);
}

constexpr std::uintptr_t StagingAddress = SELECTOR_STAGING_BASE;
constexpr std::uintptr_t StagingLimit = SELECTOR_STAGING_END;
static_assert(StagingCapacity() <= StagingLimit - StagingAddress,
              "Reviewed image set exceeds internal staging SRAM");

// Header-local catalogs have different addresses in each translation unit.
// Match the reviewed value, never a caller's pointer identity.
inline bool IsReviewedQspiImage(const Image& image) {
    for (const auto& entry : Images) {
        if (entry.execution == Execution::Qspi && image.execution == entry.execution &&
            image.size == entry.size && image.vectors.stack == entry.vectors.stack &&
            image.vectors.reset == entry.vectors.reset && image.menu_color == entry.menu_color &&
            image.path && image.sha256 && std::strcmp(image.path, entry.path) == 0 &&
            std::strcmp(image.sha256, entry.sha256) == 0) {
            return true;
        }
    }
    return false;
}

// Match staged bytes and their caller-computed digest to one reviewed entry.
inline bool VerifyImage(const std::uint8_t* bytes, std::size_t size, const Image& image,
                        const std::uint8_t* digest) {
    daisy_development::SramImageVectors vectors{};
    if (digest == nullptr || size != image.size ||
        (image.execution != Execution::Sram && image.execution != Execution::Qspi) ||
        !daisy_development::ReadImageVectors(bytes, size, 480U * 1024U,
                                             image.execution == Execution::Qspi
                                                 ? SELECTOR_QSPI_APPLICATION_BASE
                                                 : SELECTOR_AXI_SRAM_BASE,
                                             vectors) ||
        vectors.stack != image.vectors.stack || vectors.reset != image.vectors.reset) {
        return false;
    }
    constexpr char hex[] = "0123456789abcdef";
    for (std::size_t i = 0; i < 32U; ++i) {
        if (hex[digest[i] >> 4] != image.sha256[i * 2U] ||
            hex[digest[i] & 15U] != image.sha256[i * 2U + 1U]) {
            return false;
        }
    }
    return image.sha256[64] == '\0';
}
} // namespace aurora_selector
