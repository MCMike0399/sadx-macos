#pragma once
#include <array>
#include <cstdint>

namespace sadx {
// NOTSA: load-time device lookup values, before capability-dependent fog setup.
struct NinjaFogDeviceInitialState {
    std::uint32_t color = 0;
    std::uint32_t start_index = 0, end_index = 0;
    float start_value = 0, end_value = 0;
    // Stores use floating-point bit patterns; a floating-point consumer is not established.
    std::uint32_t software_density_bits = 0;
    std::array<float, 128> w_depth{}, reciprocal{};
};
// NOTSA: initial gFog range/color; the frame owner supplies its enable flag.
struct NinjaFogLevelInitialState {
    float start_z = 0, end_z = 0;
    std::uint32_t color = 0;
};
// NOTSA: initial fog values only. Device resources/capabilities, frame flags and
// saved frame ranges have separate owners. These tables precede device generation.
struct NinjaFogInitialState {
    std::array<float, 128> table{};
    NinjaFogDeviceInitialState device{};
    std::uint32_t enabled = 0, table_supported = 0, mode = 0;
    // Positive load-time caches differ from the negative runtime disable sentinel.
    float cached_start = 65535.0F, cached_end = 65535.0F;
    NinjaFogLevelInitialState level{};
};
// NOTSA: maintained initial values verified against file bytes and mapped zero fill.
NinjaFogInitialState ninja_fog_initial_state();
} // namespace sadx
