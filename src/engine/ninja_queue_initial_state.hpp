#pragma once
#include <array>
#include <cstdint>
namespace sadx {
// NOTSA: numeric load-time queue state, independent of resources and original layouts.
struct NinjaQueueInitialState {
    // Ninja comparison-table index; not a device comparison-enumeration value.
    std::int32_t depth_comparison = 1;
    // late_execMode: 0 run, 1 skip, 2 skip and draw a black screen.
    std::int32_t execution_mode = 0;
    std::int32_t lighting_palette = 0;
    std::uint8_t lighting_copy = 0, packed_blend = 0;
    std::int32_t primary_count = 0, primary_high_water = 0;
    std::int32_t secondary_count = 0, secondary_high_water = 0;
    // Float parameter groups named by their verified copy roles. Captured component
    // zero is compared against model depth; wider component meanings remain unrecovered.
    std::array<float, 3> reset_source{-3000.0F, -1200.0F, -1200.0F};
    std::array<float, 3> reset_capture{};
    std::array<float, 3> allocation_source{-3200.0F, -1600.0F, -1600.0F};
    std::array<float, 3> allocation_capture{-3000.0F, -2000.0F, -2000.0F};
};
// NOTSA: native defaults verified against pinned file data and mapped zero fill.
NinjaQueueInitialState ninja_queue_initial_state();
} // namespace sadx
