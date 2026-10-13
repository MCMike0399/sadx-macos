#pragma once
#include "cpu/x87_math.hpp"
#include "engine/ninja_matrix.hpp"
#include <cstdint>
#include <functional>
#include <optional>

namespace sadx {
// 7815e0 clamps near/far into 3d0f808/3d0f968 (stored negated); 403180 then
// caches them (90082c/900830, 3aad0c0/c4) and, only when the pair changed,
// rewrites projection-transform terms 10 and 14 via GetTransform/SetTransform(3).
struct NinjaProjectionRangeState {
    std::optional<float> near_at_3d0f808,far_at_3d0f968,x_at_90082c,cache_x_at_3aad0c0,cache_y_at_3aad0c4;
    std::optional<std::uint32_t> y_bits_at_900830;
    float negative_one_at_7dcb0c,limit_at_7dce50; // .rdata (-1.0f, -65535.0f)
};
struct NinjaTransformOwner {
    std::function<std::optional<NinjaMatrix>(std::uint32_t state)> get;          // IDirect3DDevice8::GetTransform (+0x98)
    std::function<bool(std::uint32_t state,const NinjaMatrix&)> set;            // SetTransform (+0x94)
};
enum class NinjaProjectionRangeFailure {none,unknown_fact,fp_failed,transform_unavailable};
struct NinjaProjectionRangeResult {
    NinjaProjectionRangeFailure failure=NinjaProjectionRangeFailure::none;std::uint32_t source_boundary=0;
    std::optional<X87Failure> fp_failure;bool transform_updated=false,completed=false;
    explicit operator bool()const noexcept{return failure==NinjaProjectionRangeFailure::none&&completed;}
};
NinjaProjectionRangeResult ninja_clamp_projection_range_7815e0(float near_value,float far_value,NinjaProjectionRangeState&,X87Context&);
NinjaProjectionRangeResult ninja_set_projection_range_403180(float near_value,float far_value,NinjaProjectionRangeState&,const NinjaTransformOwner&,X87Context&);
} // namespace sadx
