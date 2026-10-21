#pragma once
#include <array>
#include <cstdint>
#include <memory>

namespace sadx::texture_palette {
struct Entry {
    std::int32_t bank = -1;
    std::uint32_t slot = 0;
    std::shared_ptr<const std::array<std::uint32_t, 256>> colors;
    std::uint16_t version = 0;
};
struct State {
    // GLOBAL: SONIC 0x3d11f80 GlobalPalettes size=2048
    std::array<Entry, 128> entries = {};
    // GLOBAL: SONIC 0x3cfc000 texpalette_buffer size=1020
    std::array<std::int32_t, 255> upload_banks = {};
    // GLOBAL: SONIC 0x3d128a8 CurrentPaletteThing size=4
    std::shared_ptr<const std::array<std::uint32_t, 256>> current_colors;
    // NOTSA: native construction combines BSS zeroes with the device initializer's
    // bank/slot/colors stores; palette upload/mutation is outside this closure.
    State();
};
// FUNCTION: SONIC 0x78cf80 stSetPaletteBank
void select_bank(std::int32_t bank, State &);
} // namespace sadx::texture_palette
