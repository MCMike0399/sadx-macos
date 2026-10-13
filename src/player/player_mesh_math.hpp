#pragma once

#include "ninja/ninja_math.hpp"
#include "cpu/crt_sqrt.hpp"

namespace sadx {
// Actual450810: two binary32 edge spills followed by source-ordered cross
// components. Inputs are fully consumed before output stores, so aliasing is
// permitted. No normalization or host floating arithmetic is introduced.
void mesh_triangle_normal(const NinjaVector& first,const NinjaVector& second,
                          const NinjaVector& third,NinjaVector& output,X87Context&);
struct MeshNormalizationResult {
    std::uint32_t source_boundary = 0;
    std::optional<float> squared_spill, length, reciprocal;
    std::optional<double> sqrt_argument;
    std::optional<CrtAsinResult> sqrt;
    bool completed = false;
    explicit operator bool() const noexcept { return completed; }
};
// Actual450920/4509d0 use a binary64 argument to the original CRTsqrt, then
// binary32 length and reciprocal spills. Their zero-vector fallbacks differ.
// Unknown CRT policy returns full pending facts and leaves vector untouched.
MeshNormalizationResult normalize_mesh_vector_zero(NinjaVector&,const CoordinateAxisConstants&,
    const CrtAsinRuntime&,X87Context&);
MeshNormalizationResult normalize_mesh_vector_up(NinjaVector&,const CoordinateAxisConstants&,
    const CrtAsinRuntime&,X87Context&);
} // namespace sadx
