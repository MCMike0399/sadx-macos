#include "guest/frame_control_reference.hpp"
#include "game/game_view.hpp"
#include <iostream>
#include <stdexcept>
#include <vector>
namespace {void require(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}}
int main(){try{
    std::array<std::uint32_t,8> depth{};sadx::NinjaRenderState render;render.device_at_3d128b0=std::make_shared<sadx::NinjaRenderDevice>();render.set_render_state=[](const auto&){return sadx::NinjaRenderCallResult{true,{}};};render.packed_blend_at_3ab9890=0;
    sadx::NinjaBlendTables blend{};blend.states={{0x13,0x14}};for(unsigned i=0;i<12;++i)blend.values[i]=i+1;render.blend_tables=&blend;
    auto arena=std::make_shared<sadx::NinjaRenderQueueArena>(std::vector<std::uint8_t>(0x88000,0),0,0x88000,0);sadx::NinjaSpriteQueue queue(arena);sadx::NinjaSpriteQueueRuntime q;q.render_state=&render;
    sadx::NinjaQueueConsumerState consumer;sadx::GameLogicTimingState timing;sadx::NinjaSpriteProjection projection{};std::optional<std::array<float,4>> initial=std::array<float,4>{0,0,0,0};
    std::optional<std::uint32_t> depth_mode=0;std::array<std::optional<std::uint32_t>,3> a{},b{},c{},d{};
    sadx::NinjaQueueLifecycleOwners lifecycle{queue,q,consumer,timing,projection,initial,&depth,1.0F,-1.0F,depth_mode,a,b,c,d};
    std::optional<std::uint8_t> pending=0;std::optional<std::uint32_t> mode=7;const std::optional<std::array<float,4>> color=std::array<float,4>{0.5F,2,-2,1};
    sadx::view::SavedFovState saved_fov;
    sadx::GameViewState v{1.0F,1.0F,640.0F,480.0F,0.5F,639.0F,479.0F,pending,mode,{},0x3f800000U,{},{},{},{},{},{},0x31c7U,{},saved_fov,{},{},color};
    std::vector<std::uint32_t> calls;sadx::GameViewRuntime rt{v,lifecycle,0,[&](const sadx::GameViewCall& call){calls.push_back(call.callee);return sadx::GameLogicOwnerResult{sadx::GameLogicOwnerState::completed,0,{}};}};
    sadx::X87Context fp(0x27f);auto r=sadx::run_game_view_4208e0(rt,fp);
    require(r&&v.view_at_3b28098==640.0F&&v.view_at_3b280a4==240.0F&&projection.scale_at_3d0f9e0==1.0F&&calls==std::vector<std::uint32_t>{0x403180},"plain path: 640x480 view, no FOV call when unchanged");
    require(q.color_at_3ab9864==std::array<float,4>{0.5F,1,-1,1}&&q.captured_word_at_3d0f9c8==0x36cu&&mode==7u,"colour clamp, captured words, mode untouched");
    v.fov_at_3aad0bc=0;pending=1;calls.clear();r=sadx::run_game_view_4208e0(rt,fp);require(r&&calls==std::vector<std::uint32_t>{0x4071c0,0x7815c0,0x403180}&&mode==0u,"reset path and FOV change");
    v.screen_width_at_8928c0.reset();r=sadx::run_game_view_4208e0(rt,fp);require(r.failure==sadx::GameViewFailure::unknown_state,"unknown screen ratio");
    std::cout<<"Game view: 4208e0/420860 stores, owner calls and unknown boundaries passed.\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
