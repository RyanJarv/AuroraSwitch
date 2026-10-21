#pragma once

// Remember the chosen firmware in two alternating USB files. Only a changed
// manual choice writes a record. If power fails during a write, the other
// record remains available; both still require a valid checksum and image hash.
#include <array>
#include <cstdint>
#include <cstring>
#include "../firmware/images.hpp"

namespace aurora_selector
{
    using BootRecord = std::array<std::uint8_t, 80>;

    inline std::uint32_t RecordWord(const BootRecord& record, unsigned offset)
    {
        std::uint32_t value = 0;
        for (unsigned i = 0; i < 4; ++i)
        {
            value |= std::uint32_t(record[offset + i]) << (8 * i);
        }
        return value;
    }

    inline void RecordWord(BootRecord& record, unsigned offset, std::uint32_t value)
    {
        for (unsigned i = 0; i < 4; ++i)
        {
            record[offset + i] = value >> (8 * i);
        }
    }

    inline std::uint32_t RecordCrc(const BootRecord& record)
    {
        std::uint32_t crc = ~0U;
        for (unsigned i = 0; i < 76; ++i)
        {
            crc ^= record[i];
            for (unsigned bit = 0; bit < 8; ++bit)
            {
                crc = (crc >> 1) ^ ((0U - (crc & 1U)) & SELECTOR_BOOT_CRC_POLYNOMIAL);
            }
        }
        return ~crc;
    }

    inline bool RecordValid(const BootRecord& record)
    {
        return std::memcmp(record.data(), "AURBOOT1", 8) == 0
            && RecordWord(record, 76) == RecordCrc(record);
    }

    inline int RecordChoice(const BootRecord& record)
    {
        if (!RecordValid(record))
        {
            return -1;
        }
        for (unsigned i = 0; i < sizeof(Images) / sizeof(Images[0]); ++i)
        {
            if (std::memcmp(record.data() + 12, Images[i].sha256, 64) == 0)
            {
                return static_cast<int>(i);
            }
        }
        return -1;
    }

    struct BootChoice
    {
        int index = -1, slot = -1;
        std::uint32_t generation = 0;
    };

    template <class Store> BootChoice ReadBootChoice(Store& store)
    {
        BootRecord records[2]{};
        bool valid[2]{};
        for (unsigned slot = 0; slot < 2; ++slot)
        {
            if (store.Read(slot, records[slot]))
            {
                valid[slot] = RecordValid(records[slot]);
            }
        }
        if (!store.Healthy())
        {
            return {};
        }
        int slot = valid[0] ? 0 : valid[1] ? 1 : -1;
        if (valid[0] && valid[1])
        {
            const auto delta = RecordWord(records[1], 8) - RecordWord(records[0], 8);
            // Generations wrap around. Conflicting records with the same generation,
            // or exactly half a counter cycle apart, have no trustworthy winner.
            if ((delta == 0 && records[0] != records[1]) || delta == SELECTOR_GENERATION_HALF_RANGE)
            {
                return {-1, -2, 0};
            }
            slot = delta != 0 && delta < SELECTOR_GENERATION_HALF_RANGE ? 1 : 0;
        }
        return slot < 0
            ? BootChoice{}
            : BootChoice{RecordChoice(records[slot]), slot, RecordWord(records[slot], 8)};
    }

    template <class Store> bool RememberBootChoice(Store& store, unsigned index)
    {
        if (index >= sizeof(Images) / sizeof(Images[0]))
        {
            return false;
        }
        const auto previous = ReadBootChoice(store);
        if (!store.Healthy() || previous.slot == -2)
        {
            return false;
        }
        if (previous.index == static_cast<int>(index))
        {
            return true; // No write on repeat boots.
        }
        BootRecord record{};
        std::memcpy(record.data(), "AURBOOT1", 8);
        RecordWord(record, 8, previous.generation + 1U);
        std::memcpy(record.data() + 12, Images[index].sha256, 64);
        RecordWord(record, 76, RecordCrc(record));
        const unsigned slot = previous.slot == 0 ? 1 : 0;
        if (!store.Write(slot, record))
        {
            return false;
        }
        BootRecord readback{};
        return store.Read(slot, readback) && readback == record;
    }
} // namespace aurora_selector
