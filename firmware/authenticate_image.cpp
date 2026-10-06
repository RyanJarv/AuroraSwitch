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
    if(argc != 3 || (std::string(argv[1]) != "fdn"
                    && std::string(argv[1]) != "spectral"))
        return 2;
    const auto& image = aurora_selector::Images[std::string(argv[1]) == "fdn" ? 0 : 1];
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
    std::cout << image.sha256 << "\n";
}
