#pragma once
#include "hle/hle_xinput.hpp"
#import <GameController/GameController.h>
namespace sadx::input::mac {
// Capture one stable extended profile as the synthetic pad's raw XInput sample.
// Apple has already applied its deadzones; this is not raw physical HID parity.
std::optional<HleXInputGamepad> xinput_sample(GCController*) noexcept;
}
