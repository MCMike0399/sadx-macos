#pragma once
#include "engine/ninja_matrix.hpp"
#include "engine/ninja_projected_depth.hpp"
#include <functional>

namespace sadx::view {
struct Camera {
    std::array<float, 3> position{}, direction{};
    std::int32_t roll = 0, angle = 0;
    float near_distance = 0, far_distance = 0;
    std::array<float, 6> tail{};
};
struct State {
    Camera camera;
    NinjaMatrix view{}, view_copy{};
    D3dxMatrix perspective{}, inverse{}, device_view{};
    NinjaDepthProjection depth;
    float sprite_scale = 0, near_distance = 0, far_distance = 0;
    // File-backed clip-range defaults, distinct from the camera's negative-distance cache.
    float negative_near_distance = -1, negative_far_distance = -2049;
};
struct Services {
    NinjaMatrixStack &matrices;
    const NinjaMath::SineTable &sine;
    const D3dxMatrix &screen_transform;
    std::function<void(bool multiply, std::uint32_t transform, const D3dxMatrix &)> transform;
    SseReciprocalEstimate reciprocal;
};
enum class Failure { None, TangentOutOfRange, PerspectiveOutOfRange };
struct Result {
    Failure failure = Failure::None;
    bool inverse_stored = false;
    explicit operator bool() const { return failure == Failure::None; }
};
// FUNCTION: SONIC 0x781570 njInitCamera
void initialize_camera(Camera &);
// FUNCTION: SONIC 0x791170 Direct3D_SetProjectionMatrix
Result set_projection(float field_of_view, float far_distance, State &, Services &,
                       X87Context &, SseContext &);
// FUNCTION: SONIC 0x781250 njSetCamera
Result set_camera(State &, Services &, X87Context &, SseContext &);
struct ClipServices {
    std::function<NinjaMatrix()> get_projection;
    std::function<void(const NinjaMatrix &)> set_projection;
};
// FUNCTION: SONIC 0x7815e0 SetDrawDistance
void clamp_clip_range(float near_distance, float far_distance, State &, X87Context &);
// FUNCTION: SONIC 0x403180 ___njClipZ
bool set_clip_range(float near_distance, float far_distance, State &, ClipServices &, X87Context &);
} // namespace sadx::view
