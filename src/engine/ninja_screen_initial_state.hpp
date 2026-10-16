#pragma once
#include <array>
#include <cstdint>

namespace sadx {
// NOTSA: numeric rectangles and points, independent of original pointer layouts.
struct NinjaInitialRectangle {
    float left = 0, top = 0, right = 0, bottom = 0;
};
// NOTSA: typed storage for the inline polygon coordinates.
struct NinjaInitialScreenPoint {
    float x = 0, y = 0;
};
// NOTSA: the viewport's integer coordinates/extents and floating depth bounds.
struct NinjaViewportInitialState {
    std::uint32_t x = 0, y = 0, width = 0, height = 0;
    float min_z = 0, max_z = 0;
};
// NOTSA: positive device distances and the separately stored negative range/cache.
struct NinjaClipInitialState {
    float near_distance = 0, far_distance = 0;
    float range_near = -1.0F, range_far = -2049.0F;
    float cached_near = 0, cached_far = 0;
};
// NOTSA: load-time numeric screen/viewport data, before device or preset setup.
// Polygon resources, device identities/caps and display settings have separate owners.
struct NinjaScreenInitialState {
    NinjaClipInitialState clip{};
    // Second preset argument; broader meaning is not established.
    std::uint32_t setup_argument = 0;
    std::uint32_t flags = 0, timing_multiplier = 0;
    std::uint32_t width_pixels = 0, height_pixels = 0;
    float logical_height = 0, viewport_width = 0, viewport_height = 0;
    NinjaInitialRectangle screen_bounds{}, viewport_bounds{};
    std::uint32_t transform_invalid = 0;
    // Screen setup writes 2 here; its broader meaning is not established.
    std::uint32_t polygon_setup_word = 0, polygon_count = 0;
    std::array<NinjaInitialScreenPoint, 4> polygon_points{};
    NinjaViewportInitialState viewport{};
    float half_width = 0, half_height = 0, projection_width = 0;
    std::array<float, 16> projection{}, work_transform{};
    std::array<float, 16> base_transform{
        1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1,
    };
};
// NOTSA: maintained defaults verified against file data and mapped zero fill.
NinjaScreenInitialState ninja_screen_initial_state();
} // namespace sadx
