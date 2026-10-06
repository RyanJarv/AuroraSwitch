// Host check of the firmware's own exact-image authentication. No execution.
#include "images.hpp"
#include "../support/backed_image_reader.hpp"
#include "../support/read_only_image_staging.hpp"
#include "mbedtls/sha256.h"
#include <fstream>
#include <iostream>
#include <iterator>
#include <vector>

int main(int argc, char** argv)
{
    // Export the launch catalog instead of maintaining another host allowlist.
    // This helper is not linked into the selector application.
    if(argc == 2 && std::string(argv[1]) == "--list")
    {
        for(const auto& image : aurora_selector::Images)
            std::cout << image.path << '\t' << image.size << '\t'
                      << image.sha256 << '\t' << image.vectors.stack << '\t'
                      << image.vectors.reset << '\n';
        return 0;
    }
    if(argc != 3)
        return 2;
    std::string name = argv[1];
    // Retain the historical probe interface while accepting every catalog file.
    if(name == "fdn") name = "AR_FDN_v1_2_2.bin";
    if(name == "spectral") name = "Aurora_v1_4_4.bin";
    const aurora_selector::Image* selected = nullptr;
    for(const auto& candidate : aurora_selector::Images)
        if(name == std::string(candidate.path).substr(std::string(candidate.path).find_last_of('/') + 1))
            selected = &candidate;
    if(selected == nullptr)
        return 2;
    const auto& image = *selected;
    std::ifstream input(argv[2], std::ios::binary);
    if(!input)
        return 2;
    std::vector<std::uint8_t> bytes(image.size);
    if(!input.read(reinterpret_cast<char*>(bytes.data()), bytes.size())
       || input.peek() != std::char_traits<char>::eof())
        return 2;
    std::uint8_t digest[32]{};
    daisy_development::BackedImageDescriptor descriptor{};
    descriptor.version = descriptor.ready = descriptor.generation = 1;
    descriptor.size = bytes.size();
    if(std::strlen(image.path) >= sizeof(descriptor.path))
        return 2;
    std::strcpy(descriptor.path, image.path);
    daisy_development::BackedImageReader reader(descriptor, bytes.data(), bytes.size());
    std::vector<std::uint8_t> staged(bytes.size());
    if(daisy_development::StageReadOnlyImage(reader, image.path, staged.data(),
          staged.size(), image.size) != daisy_development::StagingResult::Staged
       || staged != bytes
       || mbedtls_sha256_ret(staged.data(), staged.size(), digest, 0) != 0
       || !aurora_selector::VerifyImage(staged.data(), staged.size(), image, digest))
        return 2;
    // Revalidation consumes the loaded buffer, not a reopened backing file.
    // Changing backing bytes cannot change the independently staged image.
    bytes.back() ^= 1;
    if(mbedtls_sha256_ret(staged.data(), staged.size(), digest, 0) != 0
       || !aurora_selector::VerifyImage(staged.data(), staged.size(), image, digest))
        return 2;
    // A post-load mutation must fail the same hash/vector predicate used by
    // both validations in handoff.cpp. This is a host check, not hardware proof.
    staged.back() ^= 1;
    if(mbedtls_sha256_ret(staged.data(), staged.size(), digest, 0) != 0
       || aurora_selector::VerifyImage(staged.data(), staged.size(), image, digest))
        return 2;
    staged.back() ^= 1;
    if(mbedtls_sha256_ret(staged.data(), staged.size(), digest, 0) != 0
       || !aurora_selector::VerifyImage(staged.data(), staged.size(), image, digest))
        return 2;
    std::cout << image.sha256 << "\n";
}
