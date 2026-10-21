// Host-only metadata export. No SHA library, SDK or firmware execution required.
#include "images.hpp"
#include <iomanip>
#include <iostream>

int main()
{
    using namespace aurora_selector;
    const auto text = [](const char* value) { std::cout << std::quoted(value); };
    std::cout << "{\"schema\":1,\"staging_capacity\":" << StagingCapacity() << ",\"colors\":[";
    bool separator = false;
    for(const auto& color : Palette)
    {
        if(separator) std::cout << ',';
        separator = true;
        std::cout << "{\"name\":"; text(color.name);
        std::cout << ",\"css\":"; text(color.css);
        std::cout << ",\"rgb\":[" << color.rgb[0] << ',' << color.rgb[1] << ',' << color.rgb[2] << "]}";
    }
    std::cout << "],\"categories\":[";
    separator = false;
    for(const auto& category : CategoryDefinitions)
    {
        if(separator) std::cout << ',';
        separator = true;
        std::cout << "{\"name\":"; text(category.name);
        std::cout << ",\"color\":"; text(GetColor(category.color).name);
        std::cout << '}';
    }
    std::cout << "],\"images\":[";
    for(std::size_t i = 0; i < ImageCount; ++i)
    {
        if(i) std::cout << ',';
        const auto& firmware = FirmwareCatalog[i];
        const auto& family = GetFamily(firmware.family);
        const auto& image = Images[i];
        std::cout << "{\"id\":"; text(firmware.id);
        std::cout << ",\"family\":"; text(family.id);
        std::cout << ",\"family_name\":"; text(family.name);
        std::cout << ",\"name\":"; text(firmware.name);
        std::cout << ",\"description\":"; text(firmware.description);
        std::cout << ",\"reference\":"; text(family.reference);
        std::cout << ",\"category\":"; text(CategoryDefinitions[Categories[i]].name);
        std::cout << ",\"color\":"; text(GetColor(family.color).name);
        std::cout << ",\"availability\":"; text(firmware.release ? "Release" : "Development");
        std::cout << ",\"download\":";
        if(firmware.download) text(firmware.download); else std::cout << "null";
        std::cout << ",\"resident\":" << (firmware.resident ? "true" : "false");
        std::cout << ",\"filename\":"; text(image.path + std::strlen("0:/aurora/"));
        std::cout << ",\"rgb\":[" << image.menu_color[0] << ',' << image.menu_color[1] << ',' << image.menu_color[2] << ']';
        std::cout << ",\"bytes\":" << image.size << ",\"sha256\":"; text(image.sha256);
        std::cout << ",\"stack\":" << image.vectors.stack << ",\"reset\":" << image.vectors.reset;
        std::cout << ",\"execution\":"; text(image.execution == Execution::Sram ? "sram" : "qspi");
        std::cout << '}';
    }
    std::cout << "]}\n";
}
