#include "game/game_loop.hpp"
#include <iostream>
#include <stdexcept>
#include <vector>
namespace {
void require(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
using sadx::GameLoopOperation;using sadx::GameLoopFailure;using sadx::GameLoopNext;
struct Fixture {
    sadx::GameLoopState state;std::optional<std::uint8_t> special=3,fog=1;sadx::GameLogicTimingState timing;sadx::GameLogicState logic{timing,{},{}};std::vector<sadx::GameLoopCall> calls;std::uint32_t keys=0,key=0;
    sadx::GameLoopRuntime runtime{state,logic,{}};
    Fixture(){
        state.exit_at_3abdf7c=0;state.focus_at_8929d4=1;state.special_flags_at_909eb4=&special;state.fog_flag_at_3abdcfe=&fog;
        state.fog_range_at_3abdca0=1;state.fog_range_at_3abdca4=2;state.fog_range_at_3abdc70=3;state.fog_range_at_3abdc74=4;
        state.request_at_3b291ad=0;state.counter_at_3b291b2=0;state.save_phase_at_3b291a4=0;state.pending_at_3abdf76=0;state.pending_at_3abdf79=0;
        state.word_at_3abdf80=0;state.mode_at_3b28114=2;state.frames_at_3abdf58=0;state.frames_at_3abdf5c=0;state.game_mode_at_3b22de4=4;state.word_at_3c85804=0;
        runtime.invoke=[this](const sadx::GameLoopCall& c)->sadx::GameLoopOwnerResult{calls.push_back(c);
            if(c.operation==GameLoopOperation::keys_40ef30)return {{sadx::GameLogicOwnerState::completed,0},keys};
            if(c.operation==GameLoopOperation::key_40ef20)return {{sadx::GameLogicOwnerState::completed,0},key};
            return {{sadx::GameLogicOwnerState::completed,0},{}};};
    }
    std::size_t count(GameLoopOperation op)const{std::size_t n=0;for(const auto& c:calls)n+=c.operation==op;return n;}
};
void tests(){
    {Fixture f;auto r=sadx::run_game_loop_startup(f.runtime);require(r&&r.next==GameLoopNext::iteration&&f.state.word_at_3abdcf8==0&&f.calls.size()==3,"startup issues 40c030, 51f680, 401410(1) then tests exit");
     require(f.calls[2].arguments[0]==1u,"401410 argument");}
    {Fixture f;auto r=sadx::run_game_loop_iteration(f.runtime);
     require(r&&r.next==GameLoopNext::iteration&&f.timing.word_at_3b11198==1&&f.state.frames_at_3abdf58==1u&&f.state.frames_at_3abdf5c==1u&&f.state.mode_at_3b28114==1u,"one focused frame");
     require(f.count(GameLoopOperation::fog_range_403180)==2&&f.calls[8].arguments[0]==1u&&f.calls[8].arguments[1]==2u,"fog range pairs pass near then far");}
    {Fixture f;f.state.focus_at_8929d4=0;auto r=sadx::run_game_loop_iteration(f.runtime);
     require(r&&f.calls.size()==4&&f.calls[3].operation==GameLoopOperation::idle_781060&&f.state.frames_at_3abdf58==0u,"unfocused frame idles without frame counters");}
    {Fixture f;f.keys=0x40;f.key=1;auto r=sadx::run_game_loop_iteration(f.runtime);require(r&&r.next==GameLoopNext::shutdown&&f.state.exit_at_3abdf7c==1u,"exit key sets the exit flag");}
    {Fixture f;f.keys=0x44;f.key=0;auto r=sadx::run_game_loop_iteration(f.runtime);require(r&&r.next==GameLoopNext::iteration&&f.state.exit_at_3abdf7c==0u,"exit query returning zero continues");}
    {Fixture f;f.state.game_mode_at_3b22de4.reset();auto r=sadx::run_game_loop_iteration(f.runtime);
     require(r.failure==GameLoopFailure::unknown_state&&r.source_boundary==0x40cc69&&f.state.frames_at_3abdf58==0u&&f.count(GameLoopOperation::frame_411d00)==1,"counter store follows the game-mode load");}
    {Fixture f;f.state.fog_range_at_3abdca0.reset();f.runtime.invoke=[&](const sadx::GameLoopCall& c)->sadx::GameLoopOwnerResult{f.calls.push_back(c);if(c.operation==GameLoopOperation::fog_411b60)f.state.fog_range_at_3abdca0=9;return {{sadx::GameLogicOwnerState::completed,0},{}};};
     auto r=sadx::run_game_loop_iteration(f.runtime);require(r.failure==GameLoopFailure::missing_return&&r.source_boundary==0x40cc84&&f.calls[8].arguments[0]==9u,"fog range reread after 411b60; keys need EAX");}
    {Fixture f;f.state.counter_at_3b291b2.reset();f.state.request_at_3b291ad=1;auto r=sadx::run_game_loop_iteration(f.runtime);
     require(r.failure==GameLoopFailure::unknown_state&&r.source_boundary==0x40cbac&&!f.state.save_phase_at_3b291a4.value_or(0),"request increment needs the counter before later stores");}
    {Fixture f;f.state.save_phase_at_3b291a4=-1;auto r=sadx::run_game_loop_iteration(f.runtime);require(r&&f.count(GameLoopOperation::save_40be40)==1&&f.state.mode_at_3b28114==2u,"save phase -1 saves and skips the idle tail");}
    {Fixture f;f.runtime.invoke={};auto r=sadx::run_game_loop_startup(f.runtime);require(r.failure==GameLoopFailure::missing_owner&&r.source_boundary==0x40cad2&&!f.state.word_at_3abdcf8,"missing first owner stops before stores");}
    {Fixture f;unsigned probes=0;f.runtime.cancellation_requested=[&]{return ++probes==4;};auto r=sadx::run_game_loop_iteration(f.runtime);
     require(r.failure==GameLoopFailure::cancelled&&r.pending_call&&r.pending_call->operation==GameLoopOperation::queue_reset_407ab0,"cancellation keeps the pending owner");}
    {Fixture f;f.state.word_at_3c85804=1;auto r=sadx::run_game_loop_shutdown(f.runtime);require(r&&r.next==GameLoopNext::finished&&f.count(GameLoopOperation::shutdown_6fbf10)==1&&f.calls.back().operation==GameLoopOperation::shutdown_77fc50,"shutdown order");
     require(f.calls[0].arguments[0]==0x7dcc30u,"first 51f680 call keeps its pushed argument");}
    {sadx::GameLoopState loop;loop.resync_at_3abdf68=0;loop.mode_at_3b28114=0;sadx::GameLogicTimingState timing;timing.frame_multiplier=3;timing.word_at_3b11198=1;sadx::GameLogicState logic{timing,{},{}};sadx::GameFrameRendererState renderer{};
     std::vector<sadx::FrameCompletionOperation> owners;sadx::FrameCompletionRuntime fc{loop,logic,renderer,sadx::FrameCompletionPolicy::original_unported,[&](const sadx::FrameCompletionCall& c){owners.push_back(c.operation);return sadx::GameLogicOwnerResult{sadx::GameLogicOwnerState::completed,0};}};
     auto r=sadx::run_frame_completion(fc);require(r.failure==sadx::FrameCompletionFailure::unported_original_policy&&r.source_boundary==0x411d1c&&owners.empty()&&timing.word_at_3b11198==1,"original lag recovery stays an explicit boundary");
     fc.policy=sadx::FrameCompletionPolicy::speedrun_2_0;r=sadx::run_frame_completion(fc);
     require(r&&r.extra_ticks==2&&timing.word_at_3b11180==2&&logic.word_at_3b11194==1&&timing.word_at_3b11198==0&&owners.size()==6&&owners.back()==sadx::FrameCompletionOperation::limiter_781060,"Speedrun 2.0 extra ticks then the limiter");
     loop.resync_at_3abdf68.reset();r=sadx::run_frame_completion(fc);require(r.failure==sadx::FrameCompletionFailure::unknown_state&&r.source_boundary==0x411d01,"unknown resync byte");}
}
}
int main(){try{tests();std::cout<<"Game loop: 40cad0 phases, source read/store order, exit tests and owner/cancel boundaries passed.\n";return 0;}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
