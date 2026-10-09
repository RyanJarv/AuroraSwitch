// Synthetic catalog, executing the same bounded discovery helper as firmware.
#include "../support/supported_image_menu.hpp"
#include <array>
#include <cassert>

int main()
{
    daisy_development::SupportedImageMenu<3> menu;
    std::array<bool, 3> authenticated{false, true, false};
    std::array<unsigned, 3> calls{};
    bool ready = true;
    auto scan = [&] {
        menu.Discover([&](std::size_t index) {
            ++calls[index]; return authenticated[index];
        }, [&] { return ready; });
    };
    assert(!menu.HasSelection());
    menu.Next();
    assert(!menu.HasSelection());
    scan();
    assert((calls == std::array<unsigned, 3>{1, 1, 1}));
    assert(menu.HasSelection() && menu.Selected() == 1);
    menu.Next();
    assert(menu.Selected() == 1); // One available image wraps to itself.
    authenticated = {true, false, true};
    scan();
    assert(menu.Selected() == 0); // Removed previous selection cannot survive.
    menu.Next(); assert(menu.Selected() == 2);
    menu.Next(); assert(menu.Selected() == 0); // Skip unavailable middle entry.
    menu.Next(); scan(); assert(menu.Selected() == 2); // Preserve valid choice.
    authenticated.fill(false); // Missing, corrupt and unsupported all reject.
    scan(); assert(!menu.HasSelection());
    authenticated.fill(true);
    scan(); assert(menu.HasSelection());
    ready = false;
    scan(); assert(!menu.HasSelection());
    ready = true;
    menu.Discover([&](std::size_t index) {
        if(index == 1) ready = false;
        return true;
    }, [&] { return ready; });
    assert(!menu.HasSelection()); // Disconnect discards even earlier matches.
    ready = true;
    menu.Discover([&](std::size_t index) {
        if(index == 2) ready = false;
        return true;
    }, [&] { return ready; });
    assert(!menu.HasSelection()); // Includes disconnect during final probe.

    // Category wrap, unavailable groups, and within-group position use firmware code.
    daisy_development::SupportedImageMenu<6> grouped({0, 1, 2, 0, 1, 2});
    std::array<bool, 6> present{true, true, true, true, false, true};
    auto discover = [&] {
        grouped.Discover([&](std::size_t index) { return present[index]; }, [] { return true; });
    };
    discover();
    assert(grouped.Selected() == 0 && grouped.Position() == 0);
    grouped.Next(); assert(grouped.Selected() == 3 && grouped.Position() == 1);
    grouped.Next(); assert(grouped.Selected() == 0);
    grouped.NextCategory(); assert(grouped.Selected() == 1 && grouped.Category() == 1);
    grouped.Next(); assert(grouped.Selected() == 1);
    grouped.NextCategory(); assert(grouped.Selected() == 2);
    grouped.Next(); assert(grouped.Selected() == 5 && grouped.Position() == 1);
    grouped.NextCategory(); assert(grouped.Selected() == 0);
    present[1] = false;
    discover(); grouped.NextCategory(); assert(grouped.Selected() == 2);
    present = {false, false, false, false, false, true};
    discover(); grouped.NextCategory(); grouped.Next();
    assert(grouped.Selected() == 5 && grouped.Position() == 0);
    grouped.Clear(); grouped.NextCategory(); assert(!grouped.HasSelection());
}
