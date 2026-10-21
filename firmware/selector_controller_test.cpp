// Replace the media implementation while exercising the selector's real flow.
#include "selector_controller.hpp"
#include <array>
#include <cassert>

struct FakeMedia
{
    bool ready = false;
    bool reject_launch_auth = false;
    bool withdraw_on_launch_auth = false;
    bool withdraw_during_discovery = false;
    std::array<bool, sizeof(aurora_selector::Images) / sizeof(aurora_selector::Images[0])>
        present{};
    unsigned attempts = 0, launches = 0, last_launch = 0;
    unsigned discovery_starts = 0, discovery_ends = 0;
    int saved = -1;
    bool remember_fail = false;
    unsigned remembers = 0;

    int BootChoice()
    {
        return saved;
    }

    bool Remember(unsigned index)
    {
        ++remembers;
        saved = index;
        return !remember_fail;
    }

    bool Ready() const
    {
        return ready;
    }

    void BeginDiscovery()
    {
        ++discovery_starts;
    }

    void EndDiscovery()
    {
        ++discovery_ends;
    }

    bool Authenticate(unsigned index)
    {
        ++attempts;
        if (withdraw_during_discovery && index == 1)
        {
            ready = false;
        }
        if (reject_launch_auth && attempts > present.size())
        {
            return false;
        }
        if (withdraw_on_launch_auth && attempts > present.size())
        {
            ready = false;
        }
        return present[index];
    }

    bool Launch(unsigned index)
    {
        ++launches;
        last_launch = index;
        return true;
    }
};

int main()
{
    using Controller = aurora_selector::SelectorController<FakeMedia>;
    FakeMedia media;
    media.present[0] = media.present[1] = media.present[2] = true;
    Controller selector(media);
    unsigned loading = 0;
    auto tick = [&](bool category = false, bool next = false, bool freeze = false)
    {
        selector.Update(category, next, freeze,
                        [&]
                        {
                            ++loading;
                        });
    };

    tick(false, false, true);
    assert(selector.GetState() == Controller::State::Waiting && media.launches == 0);
    media.ready = true;
    tick();
    assert(selector.GetState() == Controller::State::Selected && selector.Selected() == 0);
    assert(media.attempts == media.present.size());
    assert(media.discovery_starts == 1 && media.discovery_ends == 1 && loading == 1);
    tick(false, true);
    assert(selector.Selected() == 1); // Reverse stays in the current category.
    tick(true);
    assert(selector.Selected() == 2); // Shift moves to the next available category.
    tick(false, false, true);
    assert(media.launches == 1 && media.last_launch == 2);
    assert(media.attempts == media.present.size() + 1 && loading == 2);

    // A changed image fails its fresh Freeze authentication before handoff.
    FakeMedia changed;
    changed.ready = true;
    changed.present[0] = true;
    Controller changed_selector(changed);
    changed_selector.Update(false, false, false,
                            []
                            {
                            });
    changed.reject_launch_auth = true;
    changed_selector.Update(false, false, true,
                            []
                            {
                            });
    assert(changed_selector.GetState() == Controller::State::Error);
    assert(changed.launches == 0);

    // Withdrawal after authentication also prevents launch and clears the menu.
    FakeMedia withdrawn;
    withdrawn.ready = true;
    withdrawn.present[0] = true;
    Controller withdrawn_selector(withdrawn);
    withdrawn_selector.Update(false, false, false,
                              []
                              {
                              });
    withdrawn.withdraw_on_launch_auth = true;
    withdrawn_selector.Update(false, false, true,
                              []
                              {
                              });
    assert(withdrawn_selector.GetState() == Controller::State::Error);
    assert(withdrawn.launches == 0);
    withdrawn_selector.Update(false, false, false,
                              []
                              {
                              });
    assert(withdrawn_selector.GetState() == Controller::State::Waiting);
    assert(!withdrawn_selector.Menu().HasSelection());
    withdrawn.ready = true;
    withdrawn.withdraw_on_launch_auth = false;
    withdrawn_selector.Update(false, false, false,
                              []
                              {
                              });
    assert(withdrawn.discovery_starts == 2 && withdrawn_selector.Menu().HasSelection());

    withdrawn_selector.FailInitialization();
    assert(withdrawn_selector.GetState() == Controller::State::Error);
    assert(!withdrawn_selector.Menu().HasSelection());

    FakeMedia interrupted;
    interrupted.ready = true;
    interrupted.present[0] = true;
    interrupted.withdraw_during_discovery = true;
    Controller interrupted_selector(interrupted);
    interrupted_selector.Update(false, false, true,
                                []
                                {
                                });
    assert(interrupted_selector.GetState() == Controller::State::Waiting);
    assert(!interrupted_selector.Menu().HasSelection() && interrupted.launches == 0);

    for (bool menu : {false, true})
    {
        FakeMedia boot;
        boot.ready = true;
        boot.saved = 2;
        boot.present[0] = boot.present[2] = true;
        Controller automatic(boot);
        if (menu)
        {
            automatic.RequestMenu();
        }
        automatic.Update(false, false, false,
                         []
                         {
                         });
        assert(boot.launches == (menu ? 0U : 1U));
        assert(boot.remembers == 0); // Automatic startup never writes the saved choice.
        if (!menu)
        {
            assert(boot.last_launch == 2 && boot.attempts == boot.present.size() + 1);
        }
        boot.ready = false;
        automatic.Update(false, false, false,
                         []
                         {
                         });
        boot.ready = true;
        automatic.Update(false, false, false,
                         []
                         {
                         });
        assert(boot.launches == (menu ? 0U : 1U));
    }
    FakeMedia missing;
    missing.ready = true;
    missing.saved = 2;
    missing.present[0] = true;
    Controller missing_selector(missing);
    missing_selector.Update(false, false, false,
                            []
                            {
                            });
    assert(missing.launches == 0 && missing_selector.GetState() == Controller::State::Selected);
    missing.remember_fail = true;
    missing_selector.Update(false, false, true,
                            []
                            {
                            });
    assert(missing.launches == 0 && missing_selector.GetState() == Controller::State::Error);
    FakeMedia changed_boot;
    changed_boot.ready = true;
    changed_boot.saved = 0;
    changed_boot.present[0] = true;
    changed_boot.reject_launch_auth = true;
    Controller changed_auto(changed_boot);
    changed_auto.Update(false, false, false,
                        []
                        {
                        });
    assert(changed_boot.launches == 0 && changed_boot.remembers == 0);
    assert(changed_auto.GetState() == Controller::State::Error);
}
