#pragma once
// Launch API; the caller supplies fully staged bytes and their catalog entry.
#include "daisy_seed.h"
#include "images.hpp"
#include "media_transport.hpp"

namespace aurora_selector
{
    // False means rejection before irreversible teardown or flash replacement.
    // Later failure is terminal and requires reset with recovery media ready.
    bool ExperimentalLaunch(daisy::DaisySeed& seed, MediaTransport& media,
                            const Image& image, const std::uint8_t* staged);
}
