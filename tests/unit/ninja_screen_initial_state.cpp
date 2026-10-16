#include "engine/ninja_screen_initial_state.hpp"
#include "engine/ninja_projection_range.hpp"
#include <cstring>
#include <iostream>
#include <stdexcept>

namespace {
std::uint32_t bits(float value) {
    std::uint32_t word;
    std::memcpy(&word, &value, sizeof(word));
    return word;
}
void require(bool condition, const char *message) {
    if (!condition)
        throw std::runtime_error(message);
}
} // namespace
int main() {
    auto values = sadx::ninja_screen_initial_state();
    sadx::NinjaProjectionRangeState range{values.clip.near_distance,
                                          values.clip.far_distance,
                                          values.clip.range_near,
                                          values.clip.cached_near,
                                          values.clip.cached_far,
                                          bits(values.clip.range_far),
                                          -1.0F,
                                          -65535.0F};
    sadx::X87Context fp(0x023f);
    auto matrix = values.base_transform;
    unsigned reads = 0, writes = 0;
    sadx::NinjaTransformOwner transforms{
        [&](std::uint32_t state) -> std::optional<sadx::NinjaMatrix> {
            require(state == 3, "projection transform selected");
            ++reads;
            return matrix;
        },
        [&](std::uint32_t state, const sadx::NinjaMatrix &value) {
            require(state == 3, "projection transform stored");
            ++writes;
            matrix = value;
            return true;
        }};
    const auto first = sadx::ninja_set_projection_range_403180(
        values.clip.range_near, values.clip.range_far, range, transforms, fp);
    require(bool(first) && first.transform_updated && reads == 1 && writes == 1,
            "initial caches require a first projection update");
    require(range.near_at_3d0f808 == 1.0F && range.far_at_3d0f968 == 2049.0F &&
                matrix[10] == 1.00048828125F && matrix[14] == -1.00048828125F,
            "native range and base matrix feed the recovered projection arithmetic");
    const auto second = sadx::ninja_set_projection_range_403180(
        values.clip.range_near, values.clip.range_far, range, transforms, fp);
    require(bool(second) && !second.transform_updated && reads == 1 && writes == 1,
            "unchanged range uses populated caches");
    require(values.viewport.max_z == 0 && values.work_transform[0] == 0 &&
                values.projection[0] == 0,
            "load-time viewport/matrices must not use post-device defaults");
    values.base_transform[0] = 7;
    require(sadx::ninja_screen_initial_state().base_transform[0] == 1,
            "screen worlds own independent matrix values");
    std::cout << "Screen initial state: first/cached projection update and independent matrices "
                 "passed.\n";
}
