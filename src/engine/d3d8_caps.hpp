#pragma once

#include <array>
#include <cstdint>

namespace sadx {
// The shared, explicit platform assumption until the reference GPU's D3DCAPS8 is captured.
// Callers receive a Windows wire-format value; this module owns no device state.
using D3D8CapsBytes = std::array<std::uint8_t, 212>;

// NOTSA: one shared assumed record for the two maintained reference consumers.
D3D8CapsBytes assumed_d3d8_caps() noexcept;
const char* d3d8_caps_assumption() noexcept;
} // namespace sadx
