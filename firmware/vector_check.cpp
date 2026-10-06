// Host test adapter only; this does not load, execute, or authorize firmware.
#include "../support/sram_image_vectors.hpp"
#include <fstream>
#include <iostream>
#include <iterator>
#include <vector>

int main(int argc, char** argv)
{
    if(argc != 2)
        return 2;
    std::ifstream input(argv[1], std::ios::binary);
    if(!input)
        return 2;
    std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(input)), {});
    daisy_development::SramImageVectors vectors{};
    // SDK's BOOT_SRAM linker reserves the last 32 KiB of AXI SRAM.
    if(!daisy_development::ReadSramImageVectors(
           bytes.data(), bytes.size(), 480U * 1024U, vectors))
        return 2;
    std::cout << vectors.stack << " " << vectors.reset << "\n";
}
