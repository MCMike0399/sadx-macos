#pragma once
#include <optional>
#include "engine/frame_control_fwd.hpp"
#include "engine/game_logic.hpp"
#include "engine/ninja_frame_calls.hpp"
#include "engine/ninja_render.hpp"
#include "cpu/source_bytes.hpp"
#include "cpu/x87_math.hpp"
#include <array>
#include <cstdint>
#include <functional>
#include <vector>
namespace sadx {
// 791610 (device setup, reached through 781070 -> 793d80) from exact
// instructions: 7899a0(1); 3d0fae0..3d0fcdf, 3d1289c and the nine 3d0fe00
// record heads cleared; Direct3DCreate8(0xdc) -> 3d11f60 (null: 789dd0);
// 3d128b0 = 0, 7943c0 (device creation); GetDeviceCaps(3d127c0); 794ec0,
// 78d0d0, 78c5a0, 796710, 791860, 7943f0, 78c5a0; Clear; GetViewport(3d12780);
// the half extents, the -1 words, the 389d318 matrix copy and the viewport
// floats; then a tail jump to 7914b0. Device calls go to a host owner.
// Reference methods require guest/polybuff_owners.hpp; keep the primary byte owner uninstantiated.
template<> struct SourceBytes<0x3d0fae0, polybuff::kScratchWords * polybuff::kWordBytes>;
using PolygonScratchFacts = SourceBytes<0x3d0fae0, polybuff::kScratchWords * polybuff::kWordBytes>;
struct NinjaDeviceState {
    NinjaDeviceState();
    ~NinjaDeviceState();
    NinjaDeviceState(const NinjaDeviceState&) = delete;
    NinjaDeviceState& operator=(const NinjaDeviceState&) = delete;
    polybuff::Scratch polygon_scratch{};
    std::unique_ptr<PolygonScratchFacts> polygon_scratch_facts;
    PolygonScratchFacts& block_at_3d0fae0 = *polygon_scratch_facts;     // rep stosd x 0x80
    std::optional<std::uint32_t> direct3d_at_3d11f60;  // the IDirect3D8 identity
    SourceBytes<0x3d127c0,0xd4> caps_at_3d127c0;       // D3DCAPS8
    std::array<std::optional<std::uint32_t>,6> viewport_at_3d12780; // X, Y, Width, Height, MinZ, MaxZ (float bits)
    std::optional<float> half_width_at_3d12948,half_height_at_3d1294c;
    std::array<std::optional<std::uint32_t>,7> words_at_3d0fd00;
    std::array<std::optional<std::uint32_t>,16> matrix_at_3d0fd80;
    std::array<std::optional<std::uint32_t>,16> matrix_at_389d318; // .data, read only here
    std::optional<float> left_at_3d0fd20,top_at_3d0fd24,right_at_3d0fd28,bottom_at_3d0fd2c;
};
struct NinjaDeviceHost {
    std::function<std::optional<std::uint32_t>(std::uint32_t sdk)> create_direct3d;      // Direct3DCreate8 (0 = failure)
    std::function<std::optional<std::vector<std::uint8_t>>(const D3D8FrameCommand&)> caps; // vtbl +0x1c: the 0xd4 bytes written
    std::function<std::optional<std::array<std::uint32_t,6>>(const D3D8FrameCommand&)> viewport; // vtbl +0xa4 GetViewport
    std::function<std::optional<std::int32_t>(const D3D8FrameCommand&,std::uint32_t flags,std::uint32_t color,float z,std::uint32_t stencil)> clear; // vtbl +0x90
};
struct NinjaDeviceCall {std::uint32_t source_call,callee;};
struct NinjaDeviceInitRuntime {
    NinjaDeviceState& state;
    NinjaFrameCallState& frame;          // 3d1289c, the 3d0fe00 record heads
    NinjaRenderState& render;            // 3d128b0
    GameFrameRendererState& renderer;    // 7899a0's frequency/multiplier words
    X87Context& fp;
    NinjaDeviceHost host;
    // 789dd0, 7943c0 (must leave 3d128b0 set), 794ec0, 78d0d0, 796710, 791860, 7943f0, 7914b0 (tail).
    std::function<GameLogicOwnerResult(const NinjaDeviceCall&)> invoke;
};
enum class NinjaDeviceFailure {none,unknown_state,unknown_device,null_device,host_unavailable,missing_owner,owner_failed,cancelled,fp_failed,multiplier_failed};
struct NinjaDeviceResult {
    NinjaDeviceFailure failure=NinjaDeviceFailure::none;std::uint32_t source_boundary=0;std::optional<std::uint32_t> required_fact;
    std::optional<NinjaDeviceCall> pending_call;
    explicit operator bool()const noexcept{return failure==NinjaDeviceFailure::none;}
};
NinjaDeviceResult run_device_setup_791610(NinjaDeviceInitRuntime&);
} // namespace sadx
