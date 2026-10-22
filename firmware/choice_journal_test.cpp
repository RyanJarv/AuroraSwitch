#include "../support/choice_journal.hpp"
#include <cassert>
#include <cstdio>
#include <vector>
using namespace aurora_selector;
struct Driver {
    std::vector<std::uint8_t> bytes = std::vector<std::uint8_t>(0x800000, 255);
    unsigned last_write = 0, writes = 0, erases = 0, inits = 0, cut = 80, erase_cut = 4096;
    bool fail_init = false, fail_erase = false, corrupt = false, unmapped = false;
    const std::uint8_t* MappedData() { return unmapped ? nullptr : bytes.data(); }
    bool Initialize() {
        ++inits;
        return !fail_init;
    }
    bool EraseSector(unsigned offset) {
        assert(offset == 0x7e0000 || offset == 0x7e1000);
        ++erases;
        if (fail_erase)
            return false;
        std::fill(bytes.begin() + offset, bytes.begin() + offset + erase_cut, 255);
        return erase_cut == 4096;
    }
    bool WritePage(unsigned offset, std::size_t size, const std::uint8_t* data) {
        assert(offset >= 0x7e0000 && offset + size <= 0x7e2000 && offset % 256 == 0 && size == 80);
        ++writes;
        last_write = offset;
        for (unsigned i = 0; i < std::min<unsigned>(cut, size); ++i) {
            assert((bytes[offset + i] & data[i]) == data[i]);
            bytes[offset + i] &= data[i];
        }
        if (corrupt)
            bytes[offset + 20] ^= 1;
        return cut >= size;
    }
};
int main() {
    Driver initial;
    ChoiceJournal<Driver> empty(initial);
    assert(empty.Load().index == -1 && initial.inits == 0);
    assert(empty.Save(0));
    const auto installed = initial.bytes;
    // Installation deliberately leaves the journal outside the updater file.
    // Pre-existing invalid bytes must not prevent the first explicit choice.
    Driver dirty;
    std::fill(dirty.bytes.begin() + 0x7e0000, dirty.bytes.begin() + 0x7e2000, 0);
    ChoiceJournal<Driver> first_choice(dirty);
    assert(first_choice.Load().index == -1);
    assert(first_choice.Save(0));
    assert(first_choice.Load().index == 0);
    assert(first_choice.Save(1));
    assert(first_choice.Load().index == 1);
    assert(dirty.erases == 2 && dirty.writes == 2);

    // Every cut leaves either the old complete choice or the new complete one.
    // Retrying must use fresh erased space, never reprogram a torn record.
    for (unsigned cut = 0; cut <= 80; ++cut) {
        Driver d = initial;
        d.cut = cut;
        ChoiceJournal<Driver> store(d);
        const bool saved = store.Save(1);
        assert(saved == (cut == 80));
        ChoiceJournal<Driver> reboot(d);
        const auto recovered = reboot.Load().index;
        assert(recovered == 0 || recovered == 1);
        if (cut == 0)
            assert(recovered == 0);
        d.cut = 80;
        assert(reboot.Save(1));
        assert(reboot.Load().index == 1);
        assert(std::equal(d.bytes.begin(), d.bytes.begin() + 0x7e0000, installed.begin()));
        assert(std::equal(d.bytes.begin() + 0x7e2000, d.bytes.end(), installed.begin() + 0x7e2000));
    }
    Driver d;
    for (unsigned choice = 0; choice < 100; ++choice) {
        ChoiceJournal<Driver> store(d);
        assert(store.Save(choice % 2));
        const auto writes = d.writes, inits = d.inits, erases = d.erases;
        assert(store.Save(choice % 2));
        assert(store.Load().index == int(choice % 2));
        assert(d.writes == writes && d.inits == inits && d.erases == erases);
    }
    assert(d.writes == 100 && d.erases == 6); // 16 pages per sector; recycle only when full.
    d.fail_init = true;
    ChoiceJournal<Driver> unavailable(d);
    assert(!unavailable.Save(0));
    ChoiceJournal<Driver> reboot(d);
    assert(reboot.Load().index == 1);
    // Fill until an erase is next, then fail it. The other sector remains valid.
    d.fail_init = false;
    while (d.writes < 128) {
        ChoiceJournal<Driver> store(d);
        assert(store.Save(d.writes % 2));
    }
    // Power can fail after erasing an early page while later old records remain.
    // A retry must not place a new generation before those surviving records.
    const auto full = d;
    for (unsigned erased : {0U, 1U, 80U, 255U, 256U, 2048U, 4095U, 4096U}) {
        auto interrupted = full;
        interrupted.erase_cut = erased;
        ChoiceJournal<Driver> saving(interrupted);
        assert(saving.Save(0) == (erased == 4096));
        ChoiceJournal<Driver> recovered(interrupted);
        assert(recovered.Load().index == (erased == 4096 ? 0 : 1));
        interrupted.erase_cut = 4096;
        assert(recovered.Save(0));
        ChoiceJournal<Driver> next_boot(interrupted);
        assert(next_boot.Load().index == 0);
        // The committed record in the other sector survives every interrupted erase.
        assert(std::equal(interrupted.bytes.begin() + 0x7e1000,
                          interrupted.bytes.begin() + 0x7e2000, full.bytes.begin() + 0x7e1000));
    }
    d.fail_erase = true;
    ChoiceJournal<Driver> erase_failure(d);
    assert(!erase_failure.Save(0));
    ChoiceJournal<Driver> last(d);
    assert(last.Load().index == 1);
    // A successful driver return is insufficient: verify bytes and boot choice.
    Driver corrupt;
    ChoiceJournal<Driver> damaged(corrupt);
    assert(damaged.Save(12));
    const auto committed = corrupt.bytes;
    corrupt.corrupt = true;
    assert(!damaged.Save(2));
    ChoiceJournal<Driver> after_bad_write(corrupt);
    assert(after_bad_write.Load().index == 12);
    assert(!after_bad_write.Save(ImageCount));

    // Corruption must reject the newest record, never interpret its hash loosely.
    for (unsigned byte = 0; byte < 80; ++byte) {
        Driver changed;
        changed.bytes = committed;
        changed.bytes[SELECTOR_CHOICE_OFFSET + byte] ^= 1;
        ChoiceJournal<Driver> journal(changed);
        assert(journal.Load().index == -1);
    }
    auto patch = [](Driver& driver, unsigned sector, auto edit) {
        auto* bytes =
            driver.bytes.data() + SELECTOR_CHOICE_OFFSET + sector * SELECTOR_CHOICE_SECTOR_SIZE;
        BootRecord record{};
        std::memcpy(record.data(), bytes, record.size());
        edit(record);
        RecordWord(record, 76, RecordCrc(record));
        std::memcpy(bytes, record.data(), record.size());
    };
    Driver wrap;
    ChoiceJournal<Driver> wrapping(wrap);
    assert(wrapping.Save(0));
    patch(wrap, 0, [](BootRecord& record) { RecordWord(record, 8, ~0U); });
    assert(wrapping.Save(1) && wrapping.Load().index == 1);
    patch(wrap, 1, [](BootRecord& record) { record[12] = '!'; });
    assert(wrapping.Load().index == -1); // Unknown newest identity does not fall back.
    assert(wrapping.Save(2) && wrapping.Load().index == 2);

    for (auto generation : {1U, 1U + SELECTOR_GENERATION_HALF_RANGE}) {
        Driver conflict;
        ChoiceJournal<Driver> journal(conflict);
        assert(journal.Save(0) && journal.Save(1));
        patch(conflict, 1, [=](BootRecord& record) { RecordWord(record, 8, generation); });
        const auto before = conflict.bytes;
        assert(journal.Load().index == -1 && !journal.Save(2));
        assert(conflict.bytes == before && conflict.writes == 2);
    }
    Driver unreadable;
    unreadable.unmapped = true;
    ChoiceJournal<Driver> failed_read(unreadable);
    assert(failed_read.Load().index == -1 && !failed_read.Save(0));
    assert(unreadable.writes == 0 && unreadable.inits == 0);

    // Emit actual persisted bytes for an independent Python CRC/format check.
    Driver formats;
    ChoiceJournal<Driver> records(formats);
    for (unsigned image = 0; image < ImageCount; ++image) {
        assert(records.Save(image));
        assert(std::fwrite(formats.bytes.data() + formats.last_write, 1, 80, stdout) == 80);
    }
}
