#include "../support/boot_choice.hpp"
#include <cassert>
using namespace aurora_selector;
struct Store
{
    BootRecord records[2]{};
    bool present[2]{}, fail = false, corrupt = false, read_fail = false;
    unsigned writes = 0;
    bool Read(unsigned slot, BootRecord& record)
    { record = records[slot]; return !read_fail && present[slot]; }
    bool Healthy() const { return !read_fail; }
    bool Write(unsigned slot, const BootRecord& record)
    {
        ++writes;
        present[slot] = true;
        records[slot] = record;
        if(corrupt) records[slot][20] ^= 1;
        return !fail;
    }
};
int main()
{
    Store store;
    assert(ReadBootChoice(store).index == -1);
    assert(RememberBootChoice(store, 12));
    const auto first = store.records[0];
    for(unsigned boot = 0; boot < 100; ++boot)
    {
        assert(ReadBootChoice(store).index == 12);
        assert(RememberBootChoice(store, 12));
    }
    assert(store.writes == 1);
    assert(RememberBootChoice(store, 1) && store.writes == 2);
    assert(ReadBootChoice(store).index == 1 && store.records[0] == first);
    // Every single-byte corruption of the latest record preserves the old choice.
    const auto latest = store.records[1];
    for(unsigned i = 0; i < latest.size(); ++i)
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
}
