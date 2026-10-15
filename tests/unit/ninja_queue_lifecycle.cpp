#include "engine/ninja_queue_lifecycle.hpp"
#include <iostream>
#include <stdexcept>
namespace {
void require(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
struct Fixture {
    std::array<std::uint32_t,8> depth{{1,2,3,4,5,6,7,8}};sadx::NinjaRenderState render;std::vector<std::uint32_t> states;
    std::shared_ptr<sadx::NinjaRenderQueueArena> arena=std::make_shared<sadx::NinjaRenderQueueArena>(std::vector<std::uint8_t>(64,0xcd),0,64,1);sadx::NinjaSpriteQueue queue{arena};
    sadx::NinjaSpriteQueueRuntime q;sadx::NinjaQueueConsumerState consumer;sadx::GameLogicTimingState timing;sadx::NinjaSpriteProjection projection{};
    std::optional<std::array<float,4>> initial=std::array<float,4>{2,-2,0.5F,1};std::optional<std::uint32_t> depth_mode=3;
    std::array<std::optional<std::uint32_t>,3> w840{{1,2,3}},w898{},w858{{4,5,6}},w84c{};
    sadx::NinjaQueueLifecycleOwners o{queue,q,consumer,timing,projection,initial,&depth,1.0F,-1.0F,depth_mode,w840,w898,w858,w84c};
    sadx::X87Context fp{0x27f};
    Fixture(){render.device_at_3d128b0=std::make_shared<sadx::NinjaRenderDevice>();render.set_render_state=[this](const auto& c){states.push_back(c.state);return sadx::NinjaRenderCallResult{true,{}};};q.render_state=&render;}
};
}
int main(){try{
    {Fixture f;auto r=sadx::ninja_queue_allocate_408630(f.o,std::nullopt,f.fp);
     require(r&&r.alignment_assumed&&f.queue.arena()->cursor()==0&&f.queue.arena()->bytes().size()==0x88000,"allocation without a known heap address assumes alignment");
     require(f.q.color_at_3ab9864==std::array<float,4>{1,-1,0.5F,1}&&f.projection.color_argb_at_3d0f7f0==f.q.color_at_3ab9864,"402f40 clamp and 77ff90 copy");
     require(f.states==std::vector<std::uint32_t>{0x17,0xe,27}&&f.w898==f.w840&&f.w84c==f.w858&&f.timing.pending_logic_ticks==0&&f.q.mode_at_3ab98ac==0u,"407330 order and stores");
     require(bool(f.queue.bucket_front(2047))&&!f.queue.bucket_front(2047).record,"bucket sentinels initialized");}
    {Fixture f;auto r=sadx::ninja_queue_allocate_408630(f.o,0x30000008,f.fp);require(r&&!r.alignment_assumed&&f.queue.arena()->cursor()==24,"known heap address sets the 32-byte cursor offset");}
    {Fixture f;f.initial.reset();auto r=sadx::ninja_queue_allocate_408630(f.o,0,f.fp);require(r.failure==sadx::NinjaQueueLifecycleFailure::unknown_fact&&r.required_fact==0x3b18230u&&f.states.empty()&&f.w84c==f.w858,"unknown 3b18230 stops before 407330");}
    {Fixture f;f.depth_mode.reset();auto r=sadx::ninja_queue_allocate_408630(f.o,0,f.fp);require(r.failure==sadx::NinjaQueueLifecycleFailure::unknown_fact&&r.required_fact==0x8928c8u&&f.q.color_at_3ab9864,"unknown depth mode after the colour stores");}
    std::cout<<"Queue lifecycle: 408630/407ab0/407330 stores, render order, alignment policy and unknown boundaries passed.\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
