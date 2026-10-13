#pragma once
#include "engine/ninja_device_init.hpp"
#include "engine/ninja_matrix.hpp"
#include "engine/ninja_projection_range.hpp"
#include "engine/ninja_render.hpp"
#include "engine/ninja_screen.hpp"
#include "engine/ninja_sprite.hpp"
#include "math/d3dx_math.hpp"
#include <array>
#include <cstdint>
#include <optional>
namespace sadx {
// The camera/view/projection span WinMain runs at 402d4f..402d5e, from exact instructions:
//   781570 (thiscall, ECX = 3aad0a0): the camera block = position 0, direction (0, 0, -1), roll 0,
//          angle 0x2aaa, near -1.0, far -60000.0, then 1.0 / 0 / 0 / 0 / 1.0 / 0;
//   781250 (cdecl, the block): the view matrix 3d0f760 from the direction (pitch from the length,
//          yaw from x/z unless x^2 + z^2 <= 1e-6) and the roll (788290/7883c0 of -roll), its
//          translation from the position, the matrix stack reset to slot 0 (3d0f818 = 3d0fa1c,
//          depth 1) holding the view with rows 0/1/3 of columns 0/1 negated, a copy at 3d0f980,
//          the sprite scale 3d0f9e0 = width / (2 tan(angle/2)) (787e30), near/far 3d0f808/3d0f968
//          (negated block +0x20/+0x24), then 791170(angle * 2pi/65536, near, far);
//   791170 (stdcall): aspect = width / height (unsigned FILDs), vertical fov = 2 FPATAN(FPTAN(fov/2)
//          / aspect, 1), 7716df(3d12960, fovy, aspect, 1.0, far), 3d129c0 = diag(1,-1,1,1) x 3d12960
//          (770804 -> 774310), SetTransform(PROJECTION, 3d129c0), MultiplyTransform(PROJECTION,
//          3d0fd80), SetTransform(VIEW, current matrix), the current matrix copied to 389d398 and
//          its inverse (770d10 -> 7757e0, no determinant) to 389d358.
// FPTAN/FPATAN/FSINCOS use the Bochs P5/P6 models (unmeasured against the reference CPU); the D3DX
// slots assume an SSE CPU without 3DNow!, as in ninja_screen.
struct NinjaViewState {
    // 3aad0a0..3aad0bf: position x,y,z, direction x,y,z, roll, angle (raw words). +0x20/+0x24 are
    // the projection range's cache 3aad0c0/3aad0c4 (NinjaProjectionRangeState) and are borrowed.
    std::array<std::optional<std::uint32_t>,8> camera_at_3aad0a0;
    std::array<std::optional<std::uint32_t>,6> camera_tail_at_3aad0c8;         // +0x28..+0x3c
    std::array<std::optional<std::uint32_t>,16> view_at_3d0f760;               // float bits
    std::array<std::optional<std::uint32_t>,16> view_copy_at_3d0f980;
    std::array<std::optional<std::uint32_t>,16> perspective_at_3d12960;
    std::array<std::optional<std::uint32_t>,16> view_inverse_at_389d358;
    std::array<std::optional<std::uint32_t>,16> view_at_389d398;
};
struct NinjaViewRuntime {
    NinjaViewState& state;
    NinjaProjectionRangeState& range;          // 3d0f808, 3d0f968, 3aad0c0, 3aad0c4
    NinjaSpriteProjection& projection;         // 3d0f9e0
    NinjaScreenState& screen;                  // 3d129c0
    NinjaDeviceState& device_state;            // 3d12780 viewport, 3d0fd80
    NinjaRenderState& render;                  // 3d128b0
    NinjaScreenDevice& device;                 // SetTransform +0x94, MultiplyTransform +0x9c
    NinjaMatrixStack& matrix_stack;            // 3d0fa1c / 3d0f818 / 3d0fa2c
    const NinjaMath::SineTable& sine_table;    // 88af78 (788290, 7883c0)
    X87Context& fp;SseContext& sse;SseReciprocalEstimate reciprocal;
};
enum class NinjaViewFailure {none,unknown_state,unknown_device,null_device,device_unavailable,fp_out_of_range};
struct NinjaViewResult {
    NinjaViewFailure failure=NinjaViewFailure::none;std::uint32_t source_boundary=0;std::optional<std::uint32_t> required_fact;
    bool inverse_stored=false; // 7757e0 returned the inverse (false: singular, 389d358 untouched)
    explicit operator bool()const noexcept{return failure==NinjaViewFailure::none;}
};
void ninja_view_init_781570(NinjaViewState&,NinjaProjectionRangeState&);
NinjaViewResult ninja_set_view_781250(NinjaViewRuntime&);
// 791170 alone (781250 ends with it): angle in radians, near (unused by the body), far.
NinjaViewResult ninja_set_projection_791170(float fov,float near_plane,float far_plane,NinjaViewRuntime&);
} // namespace sadx
