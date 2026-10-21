#include "../support/boot_choice.hpp"
#include <cassert>
#include <algorithm>
using namespace aurora_selector;

struct Store
{
    BootRecord records[2]{};
    bool present[2]{}, fail = false, corrupt = false, read_fail = false;
    unsigned writes = 0, cut = 80;

    bool Read(unsigned slot, BootRecord& record)
    {
        record = records[slot];
        return !read_fail && present[slot];
    }

    bool Healthy() const
    {
        return !read_fail;
    }

    bool Write(unsigned slot, const BootRecord& record)
    {
        ++writes;
        present[slot] = true;
        std::copy_n(record.begin(), cut, records[slot].begin());
        if (corrupt)
        {
            records[slot][20] ^= 1;
        }
        return !fail && cut == record.size();
    }
};

int main()
{
    Store store;
    assert(ReadBootChoice(store).index == -1);
    assert(RememberBootChoice(store, 12));
    const auto first = store.records[0];
    for (unsigned boot = 0; boot < 100; ++boot)
    {
        assert(ReadBootChoice(store).index == 12);
        assert(RememberBootChoice(store, 12));
    }
    assert(store.writes == 1);
    assert(RememberBootChoice(store, 1) && store.writes == 2);
    assert(ReadBootChoice(store).index == 1 && store.records[0] == first);
    // Every single-byte corruption of the latest record preserves the old choice.
    const auto latest = store.records[1];
    for (unsigned i = 0; i < latest.size(); ++i)
    {
        store.records[1] = latest;
        store.records[1][i] ^= 1;
        assert(ReadBootChoice(store).index == 12);
    }
    store.records[1] = latest;
    store.corrupt = true;
    assert(!RememberBootChoice(store, 2));
    assert(ReadBootChoice(store).index == 1);
    store.fail = true;
    assert(!RememberBootChoice(store, 3));
    assert(!RememberBootChoice(store, 1000));
    // A valid checksum alone cannot admit an unknown firmware identity.
    store.records[1][12] = '!';
    RecordWord(store.records[1], 76, RecordCrc(store.records[1]));
    assert(ReadBootChoice(store).index == -1);
    Store wrap;
    assert(RememberBootChoice(wrap, 0));
    RecordWord(wrap.records[0], 8, ~0U);
    RecordWord(wrap.records[0], 76, RecordCrc(wrap.records[0]));
    assert(RememberBootChoice(wrap, 1));
    assert(ReadBootChoice(wrap).index == 1);
    wrap.read_fail = true;
    assert(ReadBootChoice(wrap).index == -1);
    assert(!RememberBootChoice(wrap, 2) && wrap.writes == 2);
    Store unknown;
    assert(RememberBootChoice(unknown, 0));
    assert(RememberBootChoice(unknown, 1));
    // A newer intact record for firmware removed from the catalog must not
    // silently choose the older firmware. A manual choice can replace it.
    unknown.records[1][12] = '!';
    RecordWord(unknown.records[1], 76, RecordCrc(unknown.records[1]));
    assert(ReadBootChoice(unknown).index == -1);
    assert(RememberBootChoice(unknown, 2));
    assert(ReadBootChoice(unknown).index == 2);
    Store ambiguous;
    assert(RememberBootChoice(ambiguous, 0));
    assert(RememberBootChoice(ambiguous, 1));
    RecordWord(ambiguous.records[1], 8, 1);
    RecordWord(ambiguous.records[1], 76, RecordCrc(ambiguous.records[1]));
    assert(ReadBootChoice(ambiguous).index == -1);
    assert(!RememberBootChoice(ambiguous, 2) && ambiguous.writes == 2);
    // Exhaust every interruption boundary while replacing an older slot.
    Store baseline;
    assert(RememberBootChoice(baseline, 0));
    assert(RememberBootChoice(baseline, 1));
    for (unsigned cut = 0; cut <= 80; ++cut)
    {
        auto interrupted = baseline;
        interrupted.cut = cut;
        assert(RememberBootChoice(interrupted, 2) == (cut == 80));
        assert(ReadBootChoice(interrupted).index == (cut == 80 ? 2 : 1));
    }
    for (unsigned byte = 0; byte < 80; ++byte)
    {
        for (unsigned bit = 0; bit < 8; ++bit)
        {
            auto damaged = baseline;
            damaged.records[1][byte] ^= 1U << bit;
            assert(ReadBootChoice(damaged).index == 0);
        }
    }
    auto half_range = baseline;
    RecordWord(half_range.records[1], 8, RecordWord(half_range.records[0], 8) + 0x80000000U);
    RecordWord(half_range.records[1], 76, RecordCrc(half_range.records[1]));
    assert(ReadBootChoice(half_range).index == -1);
    assert(!RememberBootChoice(half_range, 2) && half_range.writes == 2);
    // Deterministic varied/repeated choices: one write per actual change.
    unsigned random = 12345, last = 1, expected_writes = baseline.writes;
    for (unsigned i = 0; i < 100000; ++i)
    {
        random = random * 1664525U + 1013904223U;
        const unsigned next = (random >> 16) % 3;
        if (next != last)
        {
            ++expected_writes;
        }
        assert(RememberBootChoice(baseline, next));
        assert(ReadBootChoice(baseline).index == static_cast<int>(next));
        assert(baseline.writes == expected_writes);
        last = next;
    }
}
