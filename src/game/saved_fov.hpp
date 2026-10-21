#pragma once
#include <cstdint>
#include <cstring>
namespace sadx::view {
// This saved word is distinct from both the current camera FOV and the renderer
// perspective copy. Message rendering saves/restores it through actual callers.
struct SavedFovState {
    // GLOBAL: SONIC 0x3b2cbb4 LastHorizontalFOV_BAMS size=4
    std::int32_t value = 0;
};
// NOTSA: reference byte transport and legacy unsigned receipts preserve Angle bits.
inline std::uint32_t saved_fov_bits(const SavedFovState &state) noexcept {
    std::uint32_t bits;
    std::memcpy(&bits, &state.value, sizeof(bits));
    return bits;
}
inline void set_saved_fov_bits(SavedFovState &state, std::uint32_t bits) noexcept {
    std::memcpy(&state.value, &bits, sizeof(bits));
}
} // namespace sadx::view
