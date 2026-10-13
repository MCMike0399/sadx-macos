#pragma once
#include <array>
#include <cstdint>
namespace sadx {
// The shared, explicit platform assumption until the reference GPU's D3DCAPS8 is captured.
// This is a Windows wire-format value, not an owner of sonic.exe's copy at 3d127c0.
using D3D8CapsBytes=std::array<std::uint8_t,0xd4>;
D3D8CapsBytes assumed_d3d8_caps() noexcept;
const char* d3d8_caps_assumption() noexcept;
} // namespace sadx
