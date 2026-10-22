#include "selector_controller.hpp"
#include <cassert>
struct Media {
    std::array<bool, aurora_selector::ImageCount> present{};
    bool ready = true, reject = false, fail_remember = false, fail_launch = false;
    int saved = -1;
    unsigned opens = 0, auth = 0, launches = 0, writes = 0, warnings = 0, last = 0;
    bool Open(std::array<bool, aurora_selector::ImageCount>& installed) {
        ++opens;
        installed = present;
        return ready;
    }
    bool Authenticate(unsigned i) {
        ++auth;
        return present[i] && !reject;
    }
    int BootChoice() { return saved; }
    bool Remember(unsigned i) {
        ++writes;
        if (fail_remember)
            return false;
        saved = i;
        return true;
    }
    void SaveWarning() { ++warnings; }
    bool Launch(unsigned i) {
        ++launches;
        last = i;
        return !fail_launch;
    }
};
using Controller = aurora_selector::SelectorController<Media>;
void tick(Controller& c, bool category = false, bool next = false, bool freeze = false) {
    c.Update(category, next, freeze, [] {});
}
int main() {
    for (bool menu : {false, true}) {
        Media m;
        m.present[0] = m.present[2] = true;
        m.saved = 2;
        Controller c(m);
        if (menu)
            c.RequestMenu();
        tick(c);
        assert(m.launches == (menu ? 0U : 1U) && m.writes == 0);
        assert(m.auth == (menu ? 0U : 1U));
        for (unsigned idle = 0; idle < 1000; ++idle)
            tick(c);
        assert(m.opens == 1 && m.auth == (menu ? 0U : 1U));
        assert(m.launches == (menu ? 0U : 1U)); // No repeated automatic launch.
    }
    Media m;
    m.present[0] = m.present[1] = m.present[2] = true;
    Controller c(m);
    tick(c);
    tick(c, false, true);
    assert(c.Selected() == 1);
    tick(c, true, false, true);
    assert(c.Selected() == 2 && m.launches == 0);
    tick(c, false, false, true);
    assert(m.last == 2 && m.launches == 1 && m.writes == 1);
    m.reject = true;
    tick(c, false, false, true);
    assert(m.launches == 1 && m.writes == 1);
    assert(c.GetState() == Controller::State::Error);
    m.reject = false;
    m.fail_remember = true;
    m.saved = 0;
    tick(c, false, false, true);
    assert(m.launches == 2 && m.warnings == 1 && m.writes == 2);
    assert(m.saved == 0); // Failed persistence does not invent a remembered choice.
    // A failed save permits this session, but a new controller still boots
    // the previous saved choice. A later handoff failure remains an error.
    Controller after_failed_save(m);
    tick(after_failed_save);
    assert(m.last == 0 && m.writes == 2 && m.warnings == 1);
    m.fail_launch = true;
    tick(c, false, false, true);
    assert(c.GetState() == Controller::State::Error);
    assert(m.warnings == 2);
    Media missing;
    missing.present[0] = true;
    missing.saved = 2;
    Controller unavailable(missing);
    tick(unavailable);
    assert(missing.launches == 0 && unavailable.Selected() == 0);
    Media empty;
    Controller none(empty);
    tick(none, false, false, true);
    assert(none.GetState() == Controller::State::Empty && empty.launches == 0);
    Media broken;
    broken.ready = false;
    Controller invalid(broken);
    tick(invalid);
    assert(invalid.GetState() == Controller::State::Error && broken.auth == 0);
    // Slot presence does not authenticate a damaged remembered image.
    Media corrupt;
    corrupt.present[0] = corrupt.present[2] = true;
    corrupt.saved = 2;
    corrupt.reject = true;
    Controller rejected(corrupt);
    tick(rejected);
    assert(rejected.Selected() == 2 && corrupt.auth == 1);
    assert(corrupt.launches == 0 && corrupt.writes == 0);
    assert(rejected.GetState() == Controller::State::Error);
    tick(rejected); // Never automatically retry or substitute another image.
    assert(corrupt.auth == 1);
    corrupt.reject = false;
    tick(rejected, true); // User can leave the error and choose another group.
    assert(rejected.GetState() == Controller::State::Selected);
    tick(invalid, true, true, true);
    assert(broken.opens == 1 && broken.auth == 0 && broken.launches == 0);
    assert(invalid.GetState() == Controller::State::Error);
}
