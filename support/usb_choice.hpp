#pragma once
#include "../firmware/images.hpp"

namespace aurora_selector {
// One editable catalog filename, with optional LF or CRLF. This only chooses an
// entry; the complete firmware is authenticated independently before launch.
constexpr std::size_t ChoiceCapacity() {
    std::size_t maximum = 0;
    for (const auto& image : Images) {
        std::size_t length = 0;
        while (image.filename[length])
            ++length;
        if (length > maximum)
            maximum = length;
    }
    return maximum + 2; // Longest filename plus CRLF, never an unbounded text read.
}
using ChoiceRecord = std::array<std::uint8_t, ChoiceCapacity()>;
inline std::size_t EncodeChoice(unsigned index, ChoiceRecord& record) {
    if (index >= ImageCount || Images[index].launch != LaunchMethod::Ram)
        return 0;
    const auto length = std::strlen(Images[index].filename);
    std::memcpy(record.data(), Images[index].filename, length);
    record[length] = '\n';
    return length + 1;
}
inline int DecodeChoice(const ChoiceRecord& record, std::size_t size) {
    if (size == 0 || size > record.size())
        return -1;
    if (record[size - 1] == '\n') {
        --size;
        if (size && record[size - 1] == '\r')
            --size;
    }
    for (unsigned index = 0; index < ImageCount; ++index) {
        const auto* name = Images[index].filename;
        if (Images[index].launch == LaunchMethod::Ram && std::strlen(name) == size &&
            std::memcmp(record.data(), name, size) == 0)
            return static_cast<int>(index);
    }
    return -1;
}
} // namespace aurora_selector
