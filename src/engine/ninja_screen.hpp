#pragma once
#include <optional>
#include "math/d3dx_math.hpp"
#include "engine/game_frame_control.hpp"
#include "engine/ninja_device_init.hpp"
#include "engine/ninja_frame_calls.hpp"
#include "engine/ninja_keyboard.hpp"
#include "engine/ninja_sprite.hpp"
#include "engine/ninja_texture_selection.hpp"
#include <array>
#include <cstdint>
#include <functional>
#include <vector>
namespace sadx {
// Screen owners from exact instructions:
//   780c30 (781070 site 78113f, arguments config +0x14 flags, +0x18, +0x1c): the three
//          argument words, 7899a0(+0x1c), the 320/640 x 240/480 screen size (bits 0x10,
//          0x20, 0x10000), the viewport extents 3d0fa04..3d0fa10, the 3d0f820..3d0f82c
//          rectangle, 7912e0, 77ec60(0,8)/(1,6), 77ff70(-1,0) (3d0f840/3d0f9c4), 780090
//          (3ce712c = 0) and the 77dfc0 polygon block 3ce7164..3ce71a0 (with 3ce71a4/3ce71b0
//          and the 3ce7174 colours zeroed);
//   7912e0: when 3d0fd1c == 1 (only the loader's reset_device sets it), the 389d318 copy to
//          3d0fd80, the 3d0fd20..3d0fd2c rectangle, SetViewport(3d12780), SetTransform(3,
//          3d129c0) and MultiplyTransform(3, 3d0fd80);
//   78dbe0: the depth of (0, 0, w < 0x37800080 ? 65535 : 1/w) through D3DXVec3Project
//          (3d12780, 3d129c0, 389d318, 389d318);
//   77dfc0 as BeginScene's 78b922 reaches it (the loader's uiscale rewrites that call to an
//          unhooked copy of the original): 791940 (alpha by arg & 0x40), 78bba0 (texture 0
//          released), 78b780 (cull/lighting/colour-vertex and point filters), 78ea10 (a
//          TRIANGLEFAN of `count` XYZRHW|DIFFUSE vertices at 3d128ac, z from 78dbe0) and 78b800.
// Device methods are native owners a Metal backend implements; HRESULTs are ignored.
struct NinjaScreenState {
    std::optional<std::uint32_t> word_at_3d0f804,flags_at_3d0f964,word_at_3d0f814; // 780c30's arguments 2, 1, 3
    std::optional<std::uint32_t> width_at_3d0fa20,height_at_3d0f838;               // 320/640, 240/480
    std::optional<float> height_at_3d0f9e8,width_at_3d0fa04,height_at_3d0fa08;
    std::optional<std::uint32_t> word_at_3ce7160;
    std::optional<float> left_at_3d0f820,top_at_3d0f824,right_at_3d0f828,bottom_at_3d0f82c;
    // The 77dfc0 argument block {points, colours, +8, count} and its four (x, y) points.
    std::optional<std::uint32_t> points_at_3ce7164,colors_at_3ce7168,word_at_3ce716c,count_at_3ce7170;
    std::array<std::optional<std::uint32_t>,8> points_at_3ce7184;
    std::optional<std::uint32_t> invalid_at_3d0fd1c;                  // set to 1 by the loader's reset_device
    std::array<std::optional<std::uint32_t>,16> projection_at_3d129c0; // written by 791170 (engine/ninja_view)
};
struct NinjaScreenPolygon {std::uint32_t primitive_type=6,primitive_count=0,stride=0x14;std::vector<std::uint8_t> vertex_bytes;};
struct NinjaScreenDevice {
    std::function<std::optional<std::int32_t>(const D3D8FrameCommand&,const std::array<std::uint32_t,6>&)> set_viewport; // vtbl +0xa0
    std::function<std::optional<std::int32_t>(const D3D8FrameCommand&,std::uint32_t state,const D3dxMatrix&)> set_transform,multiply_transform; // +0x94, +0x9c
    std::function<std::optional<std::int32_t>(const D3D8FrameCommand&,std::uint32_t shader)> set_vertex_shader;        // +0x130
    std::function<std::optional<std::int32_t>(const D3D8FrameCommand&,const NinjaScreenPolygon&)> draw_primitive_up;   // +0x120
};
enum class NinjaScreenFailure {none,unknown_state,unknown_device,null_device,device_unavailable,render_failed,texture_failed,multiplier_failed,fp_failed,sse_failed};
struct NinjaScreenResult {
    NinjaScreenFailure failure=NinjaScreenFailure::none;std::uint32_t source_boundary=0;std::optional<std::uint32_t> required_fact;
    explicit operator bool()const noexcept{return failure==NinjaScreenFailure::none;}
};
// 7912e0's reads and device.
struct NinjaViewportRuntime {
    NinjaScreenState& state;NinjaDeviceState& device_state;NinjaRenderState& render;NinjaScreenDevice& device;X87Context& fp;
};
NinjaScreenResult run_viewport_refresh_7912e0(NinjaViewportRuntime&);
struct NinjaScreenInitRuntime {
    NinjaViewportRuntime viewport;
    GameFrameRendererState& renderer;                 // 7899a0
    std::optional<float>& width_at_3d0f9e4;           // NinjaQueueConsumerState
    NinjaSpriteProjection& projection;                // centre 3d0fa0c / 3d0fa10
    NinjaCallbackList& callbacks;                     // 780090
    std::optional<std::uint32_t>& word_at_3d0f840;std::optional<std::uint32_t>& word_at_3d0f9c4; // 77ff70 (NinjaSpriteQueueRuntime)
    std::optional<std::uint32_t>& pool_at_3ce71b0;std::optional<std::uint32_t>& size_at_3ce71a4;   // NinjaSystemInitState
    std::array<std::optional<std::uint32_t>,4>& clear_colors_at_3ce7174;                          // GameViewState
};
NinjaScreenResult run_screen_init_780c30(std::uint32_t flags,std::uint32_t word_3d0f804,std::uint32_t word_3d0f814,NinjaScreenInitRuntime&);
struct NinjaDepthRuntime {
    const NinjaScreenState& state;const NinjaDeviceState& device_state; // 3d129c0; 3d12780, 389d318
    SseContext& sse;X87Context& fp;SseReciprocalEstimate reciprocal;
};
struct NinjaDepthResult {
    std::optional<X87Value> value;std::uint32_t source_boundary=0;std::optional<std::uint32_t> required_fact;
    explicit operator bool()const noexcept{return value.has_value();}
};
NinjaDepthResult ninja_projected_depth_78dbe0(float w,NinjaDepthRuntime&);
struct NinjaPolygonRuntime {
    // Guest words of the argument block, its points and colours (callers map them to their owners).
    std::function<std::optional<std::uint32_t>(std::uint32_t address)> read;
    NinjaRenderState& render;NinjaTextureSelectionState& textures;NinjaTextureSelectionRuntime& texture_runtime;
    const std::optional<std::uint32_t>& vertex_buffer_at_3d128ac; // NinjaFrameCallState
    NinjaScreenDevice& device;NinjaDepthRuntime depth;
    std::function<bool(float)> argument_ready={};
    std::function<bool(std::uint32_t,std::uint32_t)> vertices_begin={};
    std::function<bool(const NinjaScreenPolygon&)> vertices_ready={};
    std::function<bool()> depth_begin={};
    std::function<bool(std::uint32_t,std::uint32_t,std::uint32_t)> vertex_store={};
};
struct NinjaPolygonResult {
    NinjaScreenResult status;std::optional<NinjaScreenPolygon> polygon;
    explicit operator bool()const noexcept{return bool(status);}
};
NinjaPolygonResult run_screen_polygon_77dfc0(std::uint32_t block,std::uint32_t count,float reciprocal_argument,std::uint32_t flags,NinjaPolygonRuntime&);
} // namespace sadx
