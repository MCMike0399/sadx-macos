#include "engine/ninja_frame_calls.hpp"
#include <algorithm>
#include <cstring>
#include <vector>
// Native D3D8 frame thunks with the layout of tools/verify_original_ninja_frame_calls.py.
// in: [0] thunk [1] fail-at nested call (0 none) [2] Present HRESULT [3] 3d08588 [4] 3d1289c [5] 3d128ac
//     [6] 3d0f7a4 [7] 3d0f7a8 [8] 3d0fa30 [9] 3d0fa38 [10] 3d0fa34 (0 null, else the input callback)
//     [11] 3ce712c [12..19] 3d0fa40 slots [20..35] 3d08af0 hooks [36..71] 3d0fe00 records (9 x 4 words)
//     [72] GetRenderState(0xe) value [73] GetRenderState(0x17) value [74] 3ab9890 [75] nested call that
//     rewrites 3ce712c (0 none) [76] the value it writes [77] Present policy (0 original 78ba30, 1 the loader's Direct3D_Present_r).
// out: [0] completed [1] failure [2] source boundary [3..5] 3d08588/3d1289c/3d128ac [6] 3ce712c
//      [36..71] records [80] nested count [81..] nested (callee, site, x0..x3) x 16
//      [200] device count [201..] device (method, a, b) x 32 (1 begin, 2 end, 3 present, 4 clear(flags, z), 5 get(state), 6 set(state, value))
namespace {
std::vector<std::array<std::uint32_t,6>>* g_nested=nullptr;const std::uint32_t* g_in=nullptr;sadx::NinjaCallbackList* g_callbacks=nullptr;
// Applies the same fail-at / 3ce712c rewrite script as the nested owners.
bool scripted_stop(){
    if(g_in[75]&&g_nested->size()==g_in[75])g_callbacks->count_at_3ce712c=g_in[76];
    return g_in[1]&&g_nested->size()==g_in[1];}
sadx::GameFrameControlResult input_callback(sadx::GameFrameInputState&){
    g_nested->push_back({0x411e20,0x780072,0xdeadbeef,0xdeadbeef,0xdeadbeef,0xdeadbeef});
    if(scripted_stop())return {sadx::GameFrameControlFailure::unknown_flags,0x411e20,0,false};
    return {sadx::GameFrameControlFailure::none,0,0,true};}
std::uint32_t b(float v){std::uint32_t x;std::memcpy(&x,&v,4);return x;}
}
extern "C" int sadx_native_ninja_frame_calls(const std::uint32_t* in,const std::uint8_t* blend_bytes,std::uint32_t* out){
    using namespace sadx;
    NinjaFrameCallState s;
    s.scene_flags_at_3d08588=in[3];s.device_flags_at_3d1289c=in[4];s.vertex_buffer_at_3d128ac=in[5];
    s.hook_at_3d0f7a4=in[6];s.hook_at_3d0f7a8=in[7];s.hook_at_3d0fa30=in[8];s.argument_at_3d0fa38=in[9];
    for(int i=0;i<16;++i)s.hooks_at_3d08af0[i]=in[20+i];
    for(int r=0;r<9;++r)for(int w=0;w<4;++w)s.records_at_3d0fe00[r][w]=in[36+4*r+w];
    NinjaCallbackList callbacks;callbacks.count_at_3ce712c=in[11];for(int i=0;i<8;++i)callbacks.slots_at_3d0fa40.push_back(in[12+i]);
    GameFrameRendererState renderer{};renderer.input_callback_at_3d0fa34=in[10]?&input_callback:nullptr;
    GameLogicEdgeState edges;std::optional<std::int32_t> phase=0;GameFrameInputState input{edges,{},{},phase};
    const auto tables=decode_ninja_blend_tables(blend_bytes,88);
    std::vector<std::array<std::uint32_t,3>> device;
    NinjaRenderState render;render.device_at_3d128b0=std::make_shared<NinjaRenderDevice>(NinjaRenderDevice{1});render.packed_blend_at_3ab9890=static_cast<std::uint8_t>(in[74]);render.blend_tables=tables?&*tables:nullptr;
    render.set_render_state=[&](const NinjaRenderStateCommand& c){device.push_back({6,c.state,c.value});return NinjaRenderCallResult{true,{}};};
    std::vector<std::array<std::uint32_t,6>> nested;g_nested=&nested;g_in=in;g_callbacks=&callbacks;
    NinjaFrameRuntime rt{s,callbacks,renderer,&input,render,{},in[77]?NinjaPresentPolicy::mod_loader:NinjaPresentPolicy::original,{}};
    rt.device.begin_scene=[&](const D3D8FrameCommand&){device.push_back({1,0,0});return std::optional<std::int32_t>{0};};
    rt.device.end_scene=[&](const D3D8FrameCommand&){device.push_back({2,0,0});return std::optional<std::int32_t>{0};};
    rt.device.present=[&](const D3D8FrameCommand&){device.push_back({3,0,0});return std::optional<std::int32_t>{static_cast<std::int32_t>(in[2])};};
    rt.device.clear=[&](const D3D8FrameCommand&,std::uint32_t flags,std::uint32_t color,float z,std::uint32_t stencil){device.push_back({4,flags,b(z)});device.push_back({7,color,stencil});return std::optional<std::int32_t>{0};};
    rt.device.get_render_state=[&](const D3D8FrameCommand&,std::uint32_t state){device.push_back({5,state,0});return std::optional<std::uint32_t>{state==0xe?in[72]:in[73]};};
    rt.invoke=[&](const NinjaFrameCall& c){
        nested.push_back({c.callee,c.source_call,c.ecx?*c.ecx:c.arguments[0].value_or(0xdeadbeef),c.arguments[1].value_or(0xdeadbeef),c.arguments[2].value_or(0xdeadbeef),c.arguments[3].value_or(0xdeadbeef)});
        const bool stop=scripted_stop();
        return GameLogicOwnerResult{stop?GameLogicOwnerState::unavailable:GameLogicOwnerState::completed,c.callee,{}};};
    const auto r=run_ninja_frame_call(in[0],rt);
    std::fill(out,out+320,0);out[0]=r?1U:0U;out[1]=static_cast<std::uint32_t>(r.failure);out[2]=r.source_boundary;
    const auto v=[](const auto& o){return static_cast<std::uint32_t>(o.value_or(0xdeadbeef));};
    out[3]=v(s.scene_flags_at_3d08588);out[4]=v(s.device_flags_at_3d1289c);out[5]=v(s.vertex_buffer_at_3d128ac);out[6]=v(callbacks.count_at_3ce712c);
    for(int r2=0;r2<9;++r2)for(int w=0;w<4;++w)out[36+4*r2+w]=v(s.records_at_3d0fe00[r2][w]);
    out[80]=static_cast<std::uint32_t>(nested.size());for(std::size_t i=0;i<nested.size()&&i<16;++i)std::copy(nested[i].begin(),nested[i].end(),out+81+6*i);
    out[200]=static_cast<std::uint32_t>(device.size());for(std::size_t i=0;i<device.size()&&i<32;++i)std::copy(device[i].begin(),device[i].end(),out+201+3*i);
    g_nested=nullptr;g_in=nullptr;g_callbacks=nullptr;return 1;
}
