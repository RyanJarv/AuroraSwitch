#include "../support/supported_image_menu.hpp"
#include <cassert>
int main() {
    daisy_development::SupportedImageMenu<6> menu({0, 1, 2, 0, 1, 2});
    menu.SetInstalled({true, false, true, true, false, true});
    assert(menu.Selected() == 0);
    menu.Next();
    assert(menu.Selected() == 3 && menu.Position() == 1);
    menu.NextCategory();
    assert(menu.Selected() == 2 && menu.Category() == 2);
    menu.Next();
    assert(menu.Selected() == 5 && menu.Position() == 1);
    menu.NextCategory();
    assert(menu.Selected() == 0);
    assert(!menu.Select(1) && !menu.Select(6));
    menu.SetInstalled({false, false, false, false, false, true});
    menu.NextCategory();
    menu.Next();
    assert(menu.Selected() == 5 && menu.Position() == 0);
    menu.SetInstalled({});
    menu.Next();
    menu.NextCategory();
    assert(!menu.HasSelection());
}
