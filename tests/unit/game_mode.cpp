#include "guest/game_mode_reference.hpp"
#include <iostream>
#include <stdexcept>
#include <vector>
namespace {
void require(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
struct Fixture {
    sadx::GameLoopState loop;std::optional<std::int32_t> mode=0xb;sadx::GameModeState st;std::vector<sadx::GameModeCall> calls;std::optional<std::uint32_t> menu_result=1;
    sadx::GameModeRuntime rt{loop,mode,st,{}};
    Fixture(){loop.mode_at_3b28114=0;loop.save_phase_at_3b291a4=0;loop.request_at_3b291ad=0;loop.word_at_3abdcf8=0;
        st.byte_at_3abdf74=0;st.byte_at_3abdcfc=0;st.byte_at_3abdcfd=0;st.byte_at_3b18db4=9;st.byte_at_3b2a2fa=0;st.byte_at_3b22e1e=0;st.byte_at_3abdf78=0;st.word_at_3b2c45c=0;st.word_at_3b290e4=0;st.level_word_via_3b0ef4c=7;
        rt.invoke=[this](const sadx::GameModeCall& c)->sadx::GameModeOwnerResult{calls.push_back(c);if(c.callee==0x5033b0)return {{sadx::GameLogicOwnerState::completed,0},menu_result};return {{sadx::GameLogicOwnerState::completed,0},0};};}
};
}
int main(){try{
    {Fixture f;auto r=sadx::run_game_mode_40c090(f.rt);
     require(r&&f.mode==9&&f.loop.mode_at_3b28114==1u&&f.st.byte_at_3b18db4==0&&f.calls.back().callee==0x426050,"title-menu result 1 enters mode 9 after the 0xb setup");
     require(f.calls[0].callee==0x4228f0&&f.calls[0].arguments[0]==4u&&f.calls[11].arguments[0]==0x7dcc10u,"0xb stage releases then Advertise.rel load");}
    {Fixture f;f.menu_result.reset();auto r=sadx::run_game_mode_40c090(f.rt);require(r.failure==sadx::GameModeFailure::missing_return&&r.source_boundary==0x40c51f&&f.mode==0xc,"5033b0 result is required; mode already incremented at 40c4f0");}
    {Fixture f;f.mode=0x14;f.st.byte_at_3b2a2fa=2;f.rt.invoke=[&](const sadx::GameModeCall& c)->sadx::GameModeOwnerResult{f.calls.push_back(c);return {{sadx::GameLogicOwnerState::completed,0},1u};};
     f.st.level_word_via_3b0ef4c.reset();auto r=sadx::run_game_mode_40c090(f.rt);require(r.failure==sadx::GameModeFailure::unknown_state&&r.source_boundary==0x40c839&&f.loop.mode_at_3b28114==1u,"unknown level pointer stops at its MOVSX after earlier stores");}
    {Fixture f;f.mode.reset();require(sadx::run_game_mode_40c090(f.rt).source_boundary==0x40c091,"unknown mode stops before dispatch");}
    {Fixture f;f.rt.invoke={};auto r=sadx::run_game_mode_40c090(f.rt);require(r.failure==sadx::GameModeFailure::missing_owner&&r.pending_call&&r.pending_call->callee==0x4228f0,"missing owner keeps the pending call");}
    {Fixture f;f.mode=0x19;auto r=sadx::run_game_mode_40c090(f.rt);require(r&&f.calls.size()==2&&f.calls[0].callee==0x42c4f0,"unused modes run only the tail");}
    std::cout<<"Game mode: 40c090 dispatch, required results, level-pointer and owner boundaries passed.\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
