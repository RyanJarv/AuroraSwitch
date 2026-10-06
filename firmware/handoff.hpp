#pragma once
#include "daisy_seed.h"
#include "hid/usb_host.h"
#include "images.hpp"
#ifdef SELECTOR_VIRTUAL_TRANSPORT
#include "../support/backed_image_reader.hpp"
#endif

namespace aurora_selector
{
    // Experimental only. False means pre-teardown rejection; later failure is
    // terminal and requires reset. Not a physically qualified boot contract.
    bool ExperimentalLaunch(daisy::DaisySeed& seed, daisy::USBHostHandle& usb,
                            const char* media_path, const Image& image,
                            const std::uint8_t* staged);
#ifdef SELECTOR_VIRTUAL_TRANSPORT
    // Synthetic transport only; same linked validation/teardown/copy ordering.
    bool VirtualLaunch(daisy::DaisySeed& seed,
                       volatile daisy_development::BackedImageDescriptor& descriptor,
                       const Image& image, const std::uint8_t* staged);
#endif
}
