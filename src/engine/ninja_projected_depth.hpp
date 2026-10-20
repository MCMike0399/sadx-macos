#pragma once
#include "math/d3dx_math.hpp"

namespace sadx {
// NOTSA: initialized numeric owners supplied by the device and camera systems.
struct NinjaDepthProjection {
    D3dxViewport viewport;
    D3dxMatrix projection, base_transform;
};
// NOTSA: separate calculation preserves the reference adapter's lazy missing-fact boundary.
float ninja_depth_position(float reciprocal_depth, X87Context &);
// NOTSA: source projection tail, after its screen-space position was calculated.
X87Value ninja_project_depth_position(float position, const NinjaDepthProjection &,
                                      SseContext &, X87Context &,
                                      const SseReciprocalEstimate &);
// FUNCTION: SONIC 0x78dbe0 DoWeirdProjectionThings
X87Value ninja_projected_depth(float reciprocal_depth, const NinjaDepthProjection &,
                               SseContext &, X87Context &,
                               const SseReciprocalEstimate &);
} // namespace sadx
