// Host check of the firmware's own exact-image authentication. No execution.
#include "images.hpp"
#include "mbedtls/sha256.h"
#include <fstream>
#include <iostream>
#include <iterator>
#include <vector>

int main(int argc, char** argv) {
    if (argc != 3)
        return 2;
    std::string name = argv[1];
    // Stable catalog IDs also preserve the historical fdn/spectral interface.
    const aurora_selector::Image* selected = nullptr;
    for (std::size_t i = 0; i < aurora_selector::ImageCount; ++i) {
        const auto& candidate = aurora_selector::Images[i];
        if (name == aurora_selector::FirmwareCatalog[i].id || name == candidate.filename)
            selected = &candidate;
    }
    if (selected == nullptr)
        return 2;
    const auto& image = *selected;
    std::ifstream input(argv[2], std::ios::binary);
    if (!input)
        return 2;
    std::vector<std::uint8_t> bytes(image.size);
    if (!input.read(reinterpret_cast<char*>(bytes.data()), bytes.size()) ||
        input.peek() != std::char_traits<char>::eof())
        return 2;
    std::uint8_t digest[32]{};
    std::vector<std::uint8_t> staged(bytes);
    if (mbedtls_sha256_ret(staged.data(), staged.size(), digest, 0) != 0 ||
        !aurora_selector::VerifyImage(staged.data(), staged.size(), image, digest))
        return 2;
    // Revalidation consumes the loaded buffer, not a reopened backing file.
    // Changing backing bytes cannot change the independently staged image.
    bytes.back() ^= 1;
    if (mbedtls_sha256_ret(staged.data(), staged.size(), digest, 0) != 0 ||
        !aurora_selector::VerifyImage(staged.data(), staged.size(), image, digest))
        return 2;
    // A post-load mutation must fail the same hash/vector predicate used by
    // both validations in handoff.cpp. This is a host check, not hardware proof.
    staged.back() ^= 1;
    if (mbedtls_sha256_ret(staged.data(), staged.size(), digest, 0) != 0 ||
        aurora_selector::VerifyImage(staged.data(), staged.size(), image, digest))
        return 2;
    staged.back() ^= 1;
    if (mbedtls_sha256_ret(staged.data(), staged.size(), digest, 0) != 0 ||
        !aurora_selector::VerifyImage(staged.data(), staged.size(), image, digest))
        return 2;
    std::cout << image.sha256 << "\n";
}
