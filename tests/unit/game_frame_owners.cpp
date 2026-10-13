#include "game/game_frame_owners.hpp"
#include <iostream>
#include <stdexcept>
// Owner semantics the oracle does not compare: unknown facts, nested-owner
// stops for draw lists/effects/release walks, and the key query bounds.
namespace {
using namespace sadx;
void require(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
}
int main(){try{
    std::optional<std::uint8_t> reset=1,keys=0x44;std::optional<std::uint32_t> mode=0,draws=0,effect=0;GameLogicEdgeState edges;edges.bytes_at_3b0e3e0={std::uint8_t{9},std::uint8_t{0}};edges.raw_at_3b0e3e2[4][1]=7;
    std::optional<std::array<float,4>> color=std::array<float,4>{0.5F,0.5F,0.5F,0.5F};
    GameViewState view{1.0F,1.0F,640,480,0.5F,639,479,reset,mode,0,0,0,0,0,0,0,0,0x31c7,0x31c7,0,{},color};
    NinjaRenderState render;render.device_at_3d128b0=std::make_shared<NinjaRenderDevice>(NinjaRenderDevice{1});std::vector<std::uint32_t> states;
    render.set_render_state=[&](const NinjaRenderStateCommand& c){states.push_back(c.state);return NinjaRenderCallResult{true,{}};};
    std::array<std::uint32_t,8> depth{1,2,3,4,5,6,7,8};std::optional<NinjaMath::SineTable> sine;NinjaSpriteProjection projection{};std::optional<float> width=320.0F;
    std::optional<std::array<float,4>> queue_color,projection_color;X87Context fp(0x7f);GameFrameOwnersState s;s.list_at_3b2c584=0;s.count_at_3b0e334=0;s.timer_at_3abdf88=1;s.buffer_at_3abdf84=0x1000;
    std::vector<std::uint32_t> nested;
    GameFrameOwnersRuntime rt{s,view,reset,draws,effect,keys,edges,render,&depth,sine,projection,width,color,NinjaQueueColorOwners{queue_color,projection_color,1.0F,-1.0F},fp,
        [&](const GameFrameOwnerCall& c){nested.push_back(c.callee);return GameLogicOwnerResult{GameLogicOwnerState::completed,0,{}};}};
    require(bool(run_game_frame_owner(0x40d4d0,{},rt))&&view.word_at_3b2cbb4==0x31c7u&&nested==std::vector<std::uint32_t>{0x40e2c0,0x780fb0}&&s.buffer_at_3abdf84==0u&&s.timer_at_3abdf88==0u,"40d4d0 with the 40bcf0 countdown and free");
    require(run_game_frame_owner(0x40ef20,5,rt).eax==7u&&run_game_frame_owner(0x40ef20,0,rt).eax==9u&&run_game_frame_owner(0x40ef30,{},rt).eax==0x44u,"key queries");
    require(run_game_frame_owner(0x40ef20,300,rt).failure==GameFrameOwnerFailure::unknown_state,"key index past the table");
    draws=2;nested.clear();require(bool(run_game_frame_owner(0x40d4d0,{},rt))&&nested==std::vector<std::uint32_t>{0x40d520}&&draws==0u,"draw list is a nested owner");
    effect=1;nested.clear();require(bool(run_game_frame_owner(0x40eab0,{},rt))&&nested==std::vector<std::uint32_t>{0x40eac0},"effect body is a nested owner");
    s.list_at_3b2c584=0x5000;nested.clear();require(bool(run_game_frame_owner(0x42f990,{},rt))&&s.list_at_3b2c584==0u&&nested==std::vector<std::uint32_t>{0x42f9cc},"list walk is a nested owner after the clear");
    states.clear();require(bool(run_game_frame_owner(0x420840,{},rt))&&reset==0&&states.size()==2&&queue_color==color,"420840 depth states and 4128f0 colour");
    view.fov_at_3aad0bc=0x2000;require(run_game_frame_owner(0x40d4d0,{},rt).required_fact==0x88af78u,"a FOV change needs the sine table");
    std::cout<<"Game frame owners: 40d4d0/40bcf0, key queries and bounds, nested draw/effect/walk owners, 420840 and the FOV table requirement passed.\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
