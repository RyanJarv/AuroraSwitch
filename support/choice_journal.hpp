#pragma once

// Remember exact firmware identities in two append-only flash sectors.
// A failed save leaves the other sector's committed choice available.
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include "../firmware/images.hpp"

namespace aurora_selector {
// Fixed wire format: magic, generation, exact image hash, then CRC.
constexpr char BootMagic[] = "AURBOOT1";
constexpr unsigned BootGenerationOffset = sizeof(BootMagic) - 1;
constexpr unsigned BootHashOffset = BootGenerationOffset + sizeof(std::uint32_t);
constexpr unsigned BootHashSize = 64; // Lowercase ASCII hex, not a raw 32-byte digest.
constexpr unsigned BootCrcOffset = BootHashOffset + BootHashSize;
using BootRecord = std::array<std::uint8_t, SELECTOR_CHOICE_RECORD_SIZE>;
static_assert(BootCrcOffset + sizeof(std::uint32_t) == BootRecord{}.size());
inline std::uint32_t RecordWord(const BootRecord& record, unsigned offset) {
    std::uint32_t value = 0;
    for (unsigned i = 0; i < 4; ++i)
        value |= std::uint32_t(record[offset + i]) << (8 * i);
    return value;
}
inline void RecordWord(BootRecord& record, unsigned offset, std::uint32_t value) {
    for (unsigned i = 0; i < 4; ++i)
        record[offset + i] = value >> (8 * i);
}
inline std::uint32_t RecordCrc(const BootRecord& record) {
    std::uint32_t crc = ~0U;
    for (unsigned i = 0; i < BootCrcOffset; ++i) {
        crc ^= record[i];
        for (unsigned bit = 0; bit < 8; ++bit)
            crc = (crc >> 1) ^ ((0U - (crc & 1U)) & SELECTOR_BOOT_CRC_POLYNOMIAL);
    }
    return ~crc;
}
inline bool RecordValid(const BootRecord& record) {
    return std::memcmp(record.data(), BootMagic, sizeof(BootMagic) - 1) == 0 &&
           RecordWord(record, BootCrcOffset) == RecordCrc(record);
}
struct BootChoice {
    static constexpr int Absent = -1, Ambiguous = -2;
    int index = -1, sector = Absent;
    std::uint32_t generation = 0;
};
template <class Driver> class ChoiceJournal {
  public:
    explicit ChoiceJournal(Driver& driver) : driver_(driver) {}

    BootChoice Load() {
        BootRecord records[2]{};
        bool valid[2]{};
        for (unsigned sector = 0; sector < 2; ++sector)
            valid[sector] = ReadSector(sector, records[sector]);
        if (!healthy_)
            return {};
        int sector = valid[0] ? 0 : valid[1] ? 1 : BootChoice::Absent;
        if (valid[0] && valid[1]) {
            const auto delta = RecordWord(records[1], BootGenerationOffset) -
                               RecordWord(records[0], BootGenerationOffset);
            // Counters wrap at 2^32. Conflicting equal records, or values
            // exactly halfway apart, cannot tell us which save happened last.
            if ((delta == 0 && records[0] != records[1]) || delta == SELECTOR_GENERATION_HALF_RANGE)
                return {-1, BootChoice::Ambiguous, 0};
            sector = delta != 0 && delta < SELECTOR_GENERATION_HALF_RANGE ? 1 : 0;
        }
        if (sector < 0)
            return {};
        const auto& record = records[sector]; // Already CRC-checked by ReadSector.
        int index = -1;
        for (unsigned i = 0; i < ImageCount; ++i)
            if (std::memcmp(record.data() + BootHashOffset, Images[i].sha256, BootHashSize) == 0) {
                index = static_cast<int>(i);
                break;
            }
        return {index, sector, RecordWord(record, BootGenerationOffset)};
    }

    bool Save(unsigned index) {
        if (index >= ImageCount)
            return false;
        const auto previous = Load();
        if (!healthy_ || previous.sector == BootChoice::Ambiguous)
            return false;
        if (previous.index == static_cast<int>(index))
            return true;

        BootRecord record{};
        std::memcpy(record.data(), BootMagic, sizeof(BootMagic) - 1);
        RecordWord(record, BootGenerationOffset, previous.generation + 1U);
        std::memcpy(record.data() + BootHashOffset, Images[index].sha256, BootHashSize);
        RecordWord(record, BootCrcOffset, RecordCrc(record));
        const unsigned sector = previous.sector == 0 ? 1 : 0;
        const auto* data = Sector(sector);
        if (!data)
            return false;

        // Interrupted erases can leave holes before old records. Never put
        // a newer record in those holes: append after the last used page.
        unsigned offset = 0;
        for (unsigned page = 0; page < SELECTOR_CHOICE_SECTOR_SIZE;
             page += SELECTOR_CHOICE_RECORD_STRIDE)
            if (!Erased(data + page, SELECTOR_CHOICE_RECORD_STRIDE))
                offset = page + SELECTOR_CHOICE_RECORD_STRIDE;
        const auto sector_offset = SELECTOR_CHOICE_OFFSET + sector * SELECTOR_CHOICE_SECTOR_SIZE;
        if (!driver_.Initialize())
            return Fail();
        if (offset == SELECTOR_CHOICE_SECTOR_SIZE) {
            // sector is opposite the newest valid record, or no record exists.
            if (!driver_.EraseSector(sector_offset))
                return Fail();
            data = Sector(sector);
            if (!data || !Erased(data, SELECTOR_CHOICE_SECTOR_SIZE))
                return Fail();
            offset = 0;
        }
        if (!driver_.WritePage(sector_offset + offset, record.size(), record.data()))
            return Fail();
        data = Sector(sector);
        if (!data || std::memcmp(data + offset, record.data(), record.size()) != 0)
            return Fail();
        // Check both the physical write and the choice a subsequent boot sees.
        const auto saved = Load();
        return healthy_ && saved.index == static_cast<int>(index) &&
               saved.sector == static_cast<int>(sector) &&
               saved.generation == previous.generation + 1U;
    }

  private:
    bool ReadSector(unsigned sector, BootRecord& record) {
        const auto* data = Sector(sector);
        if (!data)
            return false;
        bool found = false;
        for (unsigned offset = 0; offset < SELECTOR_CHOICE_SECTOR_SIZE;
             offset += SELECTOR_CHOICE_RECORD_STRIDE) {
            BootRecord candidate{};
            std::memcpy(candidate.data(), data + offset, candidate.size());
            if (!RecordValid(candidate))
                continue;
            if (found) {
                const auto delta = RecordWord(candidate, BootGenerationOffset) -
                                   RecordWord(record, BootGenerationOffset);
                if (delta == 0 || delta >= SELECTOR_GENERATION_HALF_RANGE)
                    return Fail();
            }
            record = candidate;
            found = true;
        }
        return found;
    }
    const std::uint8_t* Sector(unsigned sector) {
        const auto* mapped = driver_.MappedData();
        if (!healthy_ || !mapped) {
            Fail();
            return nullptr;
        }
        return mapped + SELECTOR_CHOICE_OFFSET + sector * SELECTOR_CHOICE_SECTOR_SIZE;
    }
    bool Fail() {
        healthy_ = false;
        return false;
    }
    static bool Erased(const std::uint8_t* bytes, unsigned size) {
        return std::all_of(bytes, bytes + size, [](std::uint8_t value) { return value == 255; });
    }
    Driver& driver_;
    bool healthy_ = true;
};
} // namespace aurora_selector
