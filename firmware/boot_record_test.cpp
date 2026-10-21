// Emit real records for an independent Python CRC/format check; no payload bytes.
#include "../support/boot_choice.hpp"
#include <cassert>
#include <cstdio>
using namespace aurora_selector;

struct RecordStore
{
    BootRecord records[2]{};
    bool present[2]{};
    unsigned writes = 0;

    bool Healthy() const
    {
        return true;
    }

    bool Read(unsigned slot, BootRecord& record)
    {
        record = records[slot];
        return present[slot];
    }

    bool Write(unsigned slot, const BootRecord& record)
    {
        ++writes;
        present[slot] = true;
        records[slot] = record;
        return true;
    }
};

int main()
{
    constexpr unsigned count = sizeof(Images) / sizeof(Images[0]);
    static_assert(count > 1);
    for (unsigned index = 0; index < count; ++index)
    {
        for (auto generation : {0U, 1U, 0xfffffffeU, 0xffffffffU})
        {
            RecordStore store;
            assert(RememberBootChoice(store, (index + 1) % count));
            RecordWord(store.records[0], 8, generation);
            RecordWord(store.records[0], 76, RecordCrc(store.records[0]));
            assert(RememberBootChoice(store, index));
            assert(ReadBootChoice(store).index == static_cast<int>(index));
            assert(RememberBootChoice(store, index) && store.writes == 2);
            assert(std::fwrite(store.records[1].data(), 1, 80, stdout) == 80);
        }
    }
}
