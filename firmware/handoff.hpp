#pragma once
// Start a firmware from the verified RAM buffer and its approved catalog entry.
#include "daisy_seed.h"
#include "images.hpp"
#include "media_transport.hpp"

namespace aurora_selector {
// False means the selector can still show an error and accept another choice.
// Once cleanup or flash replacement prevents a safe return, stop instead;
// recovery then requires restarting with the recovery USB drive ready.
bool ExperimentalLaunch(daisy::DaisySeed& seed, MediaTransport& media, const Image& image,
                        const std::uint8_t* staged);
} // namespace aurora_selector
