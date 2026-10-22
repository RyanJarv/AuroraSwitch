#pragma once
// Launch API; the caller supplies fully staged bytes and a catalog index.
#include "daisy_seed.h"
#include "images.hpp"

namespace aurora_selector {
// False means rejection before irreversible teardown or flash replacement.
// Later failure is terminal and requires reset with recovery media ready.
bool LaunchFirmware(daisy::DaisySeed& seed, unsigned index, const std::uint8_t* staged);
} // namespace aurora_selector
