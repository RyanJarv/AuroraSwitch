#include "../support/choice_journal.hpp"
#include "selector_controller.hpp"
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

// Exercise the actual controller and journal together. Only the flash storage
// and terminal image launch are substitutes; no physical timing is modeled.
struct JournalMedia {
    explicit JournalMedia(Driver& flash) : journal(flash) {}
    ChoiceJournal<Driver> journal;
    unsigned launches = 0, warnings = 0;
    int launched = -1;
    bool Open(std::array<bool, ImageCount>& installed) {
        installed.fill(false);
        installed[0] = installed[1] = true;
        return true;
    }
    bool Authenticate(unsigned index) { return index < 2; }
    int BootChoice() { return journal.Load().index; }
    bool Remember(unsigned index) { return journal.Save(index); }
    void SaveWarning() { ++warnings; }
    bool Launch(unsigned index) {
        ++launches;
        launched = index;
        return true;
    }
};

void TestBootWriteBudget() {
    Driver flash;
    using Controller = SelectorController<JournalMedia>;
    auto tick = [](Controller& controller, bool next = false, bool freeze = false) {
        controller.Update(false, next, freeze, [] {});
    };
    for (unsigned cycle = 0; cycle < 100; ++cycle) {
        const unsigned selected = cycle % 2;
        const auto writes = flash.writes, erases = flash.erases;
        JournalMedia menu{flash};
        Controller manual(menu);
        manual.RequestMenu();
        tick(manual);
        if (selected == 1)
            tick(manual, true);
        assert(manual.Selected() == selected);
        tick(manual, false, true);
        assert(menu.launched == int(selected) && menu.warnings == 0);
        assert(flash.writes == writes + 1);
        assert(flash.erases == erases + (cycle >= 32 && cycle % 32 < 2 ? 1U : 0U));
        const auto after = flash.bytes;
        const auto inits = flash.inits, saved_erases = flash.erases;

        for (unsigned boot = 0; boot < 3; ++boot) {
            JournalMedia media{flash};
            Controller automatic(media);
            for (unsigned idle = 0; idle < 100; ++idle)
                tick(automatic);
            assert(media.launches == 1 && media.launched == int(selected));
        }
        // Explicitly choosing the same image is also a read-only operation.
        tick(manual, false, true);
        assert(flash.writes == writes + 1 && flash.erases == saved_erases);
        assert(flash.inits == inits && flash.bytes == after);
    }
    assert(flash.writes == 100 && flash.erases == 6);

    // Save failures must not become automatic retries. A new boot uses the
    // previous committed choice; additional idle updates perform no writes.
    for (unsigned failure = 0; failure < 3; ++failure) {
        Driver broken;
        ChoiceJournal<Driver> seed(broken);
        for (unsigned i = 0; i < 32; ++i)
            assert(seed.Save(i % 2)); // Both sectors full, last choice is 1.
        broken.fail_init = failure == 0;
        broken.fail_erase = failure == 1;
        broken.cut = failure == 2 ? 0 : 80;
        const auto writes = broken.writes, erases = broken.erases, inits = broken.inits;
        JournalMedia media{broken};
        Controller manual(media);
        manual.RequestMenu();
        tick(manual);
        tick(manual, false, true); // Choose 0; the save fails, launch still allowed.
        assert(media.warnings == 1 && media.launched == 0);
        assert(broken.inits == inits + 1);
        assert(broken.erases == erases + (failure == 0 ? 0U : 1U));
        assert(broken.writes == writes + (failure == 2 ? 1U : 0U));
        const auto after = broken.bytes;
        const auto failed_writes = broken.writes, failed_erases = broken.erases;
        for (unsigned idle = 0; idle < 1000; ++idle)
            tick(manual);
        JournalMedia rebooted{broken};
        Controller automatic(rebooted);
        for (unsigned idle = 0; idle < 1000; ++idle)
            tick(automatic);
        assert(rebooted.launched == 1 && rebooted.launches == 1);
        assert(broken.writes == failed_writes && broken.erases == failed_erases);
        assert(broken.inits == inits + 1 && broken.bytes == after);
    }
}

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

    TestBootWriteBudget();

    // Emit actual persisted bytes for an independent Python CRC/format check.
    Driver formats;
    ChoiceJournal<Driver> records(formats);
    for (unsigned image = 0; image < ImageCount; ++image) {
        assert(records.Save(image));
        assert(std::fwrite(formats.bytes.data() + formats.last_write, 1, 80, stdout) == 80);
    }
}
