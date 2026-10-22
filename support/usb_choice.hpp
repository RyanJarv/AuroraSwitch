#pragma once
#include "../firmware/images.hpp"

namespace aurora_selector {
// A USB record identifies exact bytes, not a menu position that can move when
// the catalog changes. CRC rejects interrupted/corrupt saves; it is not a
// substitute for authenticating the actual firmware before launch.
using ChoiceRecord = std::array<std::uint8_t, SELECTOR_CHOICE_FILE_SIZE>;
constexpr char ChoiceMagic[] = "AURUSB01";
inline std::uint32_t ChoiceCrc(const std::uint8_t* data, std::size_t size) {
    std::uint32_t crc = SELECTOR_U32_MASK;
    for (std::size_t i = 0; i < size; ++i) {
        crc ^= data[i];
        for (unsigned bit = 0; bit < 8; ++bit)
            crc = (crc >> 1) ^ ((crc & 1U) ? SELECTOR_BOOT_CRC_POLYNOMIAL : 0U);
    }
    return ~crc;
}
inline ChoiceRecord EncodeChoice(unsigned index) {
    ChoiceRecord record{};
    if (index >= ImageCount)
        return record;
    std::memcpy(record.data(), ChoiceMagic, 8);
    std::memcpy(record.data() + 8, Images[index].sha256, 64);
    const auto crc = ChoiceCrc(record.data(), record.size() - 4);
    for (unsigned byte = 0; byte < 4; ++byte)
        record[record.size() - 4 + byte] = static_cast<std::uint8_t>(crc >> (8 * byte));
    return record;
}
inline int DecodeChoice(const ChoiceRecord& record) {
    std::uint32_t stored = 0;
    for (unsigned byte = 0; byte < 4; ++byte)
        stored |= std::uint32_t(record[record.size() - 4 + byte]) << (8 * byte);
    if (std::memcmp(record.data(), ChoiceMagic, 8) != 0 ||
        stored != ChoiceCrc(record.data(), record.size() - 4))
        return -1;
    for (unsigned index = 0; index < ImageCount; ++index)
        if (std::memcmp(record.data() + 8, Images[index].sha256, 64) == 0)
            return static_cast<int>(index);
    return -1;
}
} // namespace aurora_selector
