#include "engine/ninja_projected_depth.hpp"
#include "cpu/fidelity_float.hpp"
#include <cstring>

namespace sadx {
float ninja_depth_position(float reciprocal_depth, X87Context &fp) {
    using fidelity::ExactFloat;
    if (ExactFloat(fp, reciprocal_depth).compare(1.5259022e-05f) == X87Comparison::less)
        return 65535.0f;
    return (ExactFloat(fp, 1.0f) / reciprocal_depth).rounded();
}
X87Value ninja_project_depth_position(float position, const NinjaDepthProjection &state,
                                      SseContext &sse, X87Context &fp,
                                      const SseReciprocalEstimate &reciprocal) {
    std::uint32_t position_bits;
    std::memcpy(&position_bits, &position, sizeof(position));
    const auto projected = d3dx_vec3_project_772615(
        {0, 0, position_bits}, state.viewport, D3dxMatrixArgument{state.projection, true},
        D3dxMatrixArgument{state.base_transform, false},
        D3dxMatrixArgument{state.base_transform, false}, sse, fp, reciprocal);
    float depth;
    std::memcpy(&depth, &projected[2], sizeof(depth));
    return fp.from_float(depth);
}
X87Value ninja_projected_depth(float reciprocal_depth, const NinjaDepthProjection &state,
                               SseContext &sse, X87Context &fp,
                               const SseReciprocalEstimate &reciprocal) {
    const auto position = ninja_depth_position(reciprocal_depth, fp);
    return ninja_project_depth_position(position, state, sse, fp, reciprocal);
}
} // namespace sadx
