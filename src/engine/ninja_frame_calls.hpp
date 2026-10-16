#pragma once
#include <optional>
#include "engine/frame_control_fwd.hpp"
#include "engine/game_logic.hpp"
#include "engine/ninja_keyboard.hpp"
#include "engine/ninja_render.hpp"
#include <array>
#include <cstdint>
#include <functional>
#include <optional>
namespace sadx {
// 411d00's D3D8 frame thunks, each from exact instructions:
//   781010 -> 78bab0: 780030 (the 3d0fa40 callback list, then the 3d0f7a4,
//             3d0f7a8, 3d0fa34 and 3d0fa30(3d0fa38) hooks) + 7959a0 (the 16
//             hooks at 3d08af0..3d08b2c);
//   781020 -> 78b960 EndScene path, 781030 -> 78ba30 Present,
//   781040 -> 78ba50 Clear (then 794800 on the nine 3d0fe00 vertex-buffer records),
//   781050 -> 78b880 BeginScene path.
// The device methods are native owners a Metal backend implements. Unported
// callees (7912e0, 793cc0, 77dfc0, 789dd0, callback addresses) are nested owners.
struct D3D8FrameCommand {NinjaRenderDeviceHandle device;std::uint32_t source_call;};
// Each returns the HRESULT the backend reports; nullopt = backend unavailable.
// The source ignores every HRESULT except Present's (compared with DEVICELOST).
struct D3D8FrameDevice {
    std::function<std::optional<std::int32_t>(const D3D8FrameCommand&)> begin_scene,end_scene; // vtbl +0x88, +0x8c
    std::function<std::optional<std::int32_t>(const D3D8FrameCommand&)> present;               // vtbl +0x3c, four NULLs
    // vtbl +0x90 Clear(0, NULL, flags, color, z, stencil).
    std::function<std::optional<std::int32_t>(const D3D8FrameCommand&,std::uint32_t flags,std::uint32_t color,float z,std::uint32_t stencil)> clear;
    // vtbl +0xcc GetRenderState: the value written to the caller's local.
    std::function<std::optional<std::uint32_t>(const D3D8FrameCommand&,std::uint32_t state)> get_render_state;
};
// 78ba30 as the profile runs it. The pinned mod loader (eb4cdcca,
// SADXModLoader direct3d.cpp) WriteJumps 78ba30 to Direct3D_Present_r: Present
// with four NULLs, direct3d::reset_device on DEVICELOST, then the
// onRenderSceneStart events (no enabled mod exports OnRenderSceneStart).
enum class NinjaPresentPolicy {original,mod_loader};
constexpr std::int32_t kD3DErrDeviceLost=static_cast<std::int32_t>(0x88760868u);
struct NinjaFrameCallState {
    std::optional<std::uint32_t> scene_flags_at_3d08588;  // bit 0: inside BeginScene..EndScene
    std::optional<std::uint32_t> device_flags_at_3d1289c; // bit 0 adds D3DCLEAR_STENCIL
    std::optional<std::uint32_t> vertex_buffer_at_3d128ac;
    std::optional<std::uint32_t> hook_at_3d0f7a4,hook_at_3d0f7a8,hook_at_3d0fa30,argument_at_3d0fa38;
    std::array<std::optional<std::uint32_t>,16> hooks_at_3d08af0;
    // 3d0fe00 + 0x24*i: +0 vertex buffer (native handle, 0 = none), +4, +8, +0xc words.
    std::array<std::array<std::optional<std::uint32_t>,4>,9> records_at_3d0fe00;
};
// Nested owner call. `ecx` is set for thiscall/register callees (793cc0 ECX=1).
struct NinjaFrameCall {std::uint32_t source_call,callee;std::optional<std::uint32_t> ecx;std::array<std::optional<std::uint32_t>,4> arguments={};};
struct NinjaFrameRuntime {
    NinjaFrameCallState& state;
    NinjaCallbackList& callbacks;              // 3ce712c, 3d0fa40 (77ffc0's list)
    GameFrameRendererState& renderer;          // 3d0fa34: 411e30's native input callback
    GameFrameInputState* input=nullptr;        // what that callback reads
    NinjaRenderState& render;                  // device 3d128b0, SetRenderState, 77ec60's tables
    D3D8FrameDevice device;
    NinjaPresentPolicy present_policy=NinjaPresentPolicy::mod_loader;
    std::function<GameLogicOwnerResult(const NinjaFrameCall&)> invoke;
};
enum class NinjaFrameFailure {none,unknown_state,unknown_device,null_device,device_unavailable,render_failed,
    missing_owner,owner_failed,cancelled,index_out_of_range,input_failed,not_a_frame_call};
struct NinjaFrameResult {
    NinjaFrameFailure failure=NinjaFrameFailure::none;
    std::uint32_t source_boundary=0;
    std::optional<std::uint32_t> required_fact;
    std::optional<NinjaFrameCall> pending_call;
    std::optional<std::int32_t> present_hresult;
    explicit operator bool()const noexcept{return failure==NinjaFrameFailure::none;}
};
// thunk: 0x781010..0x781050.
NinjaFrameResult run_ninja_frame_call(std::uint32_t thunk,NinjaFrameRuntime&);
// Actual 7959c0: keep the table unchanged when `hook` is already present; otherwise
// store it in the first zero slot of 3d08af0..3d08b2c (nothing when all 16 are used).
NinjaFrameResult ninja_hook_register_7959c0(NinjaFrameCallState&,std::uint32_t hook);
} // namespace sadx
