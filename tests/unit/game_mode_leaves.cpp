#include "guest/game_mode_leaves_reference.hpp"
#include <iostream>
#include <stdexcept>
// Semantics the oracle does not compare: unknown facts, the per-character
// table bound, nested-owner stops and writes through borrowed owners.
namespace {
using namespace sadx;
void require(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
struct Fixture {
    GameModeState mode;GameLogicTimingState timing;std::optional<std::int32_t> cutscene=5;std::optional<TaskHandle> event;CrtRandState rng;std::optional<std::uint16_t> game_mode,stage=0;
    GameModeLeafState s{mode,timing,cutscene,event,rng,game_mode,stage};
    Fixture(){s.character_at_3b22dc0=0;s.byte_at_3b0ef34=1;for(auto& b:s.block_at_3b2b5f9.bytes)b=2;for(auto& b:s.block_at_90ed54.bytes)b=0;for(auto& b:s.block_at_3b290c4.bytes)b=1;
        for(auto& b:s.block_at_3b18250.bytes)b=0x11;for(auto& b:s.block_at_3c4ae08.bytes)b=0x22;}
};
GameLogicOwnerResult ok(const GameModeLeafCall&){return {GameLogicOwnerState::completed,0,{}};}
}
int main(){try{
    {Fixture f;f.s.character_at_3b22dc0.reset();const auto r=run_game_mode_leaf(0x4144e0,{},f.s,ok);require(r.outcome==GameModeLeafOutcome::unknown_state&&r.required_fact==0x3b22dc0u,"unknown character");}
    {Fixture f;f.mode.byte_at_3b2a2fa=7;require(run_game_mode_leaf(0x42cea0,{},f.s,ok).outcome==GameModeLeafOutcome::completed&&f.mode.byte_at_3b2a2fa==0,"42cea0 writes the borrowed 3b2a2fa byte");}
    {Fixture f;f.s.character_at_3b22dc0=9;const auto r=run_game_mode_leaf(0x420e90,9,f.s,ok);require(r.outcome==GameModeLeafOutcome::unknown_state&&r.required_fact==0x90ed54u+36,"table index past the 8 characters");}
    {Fixture f;f.s.block_at_90ed54.write(0x90ed54,0x1234,4);std::uint32_t site=0;
        const auto r=run_game_mode_leaf(0x420e90,0,f.s,[&](const GameModeLeafCall& c){site=c.source_call;return GameLogicOwnerResult{GameLogicOwnerState::unavailable,c.callee,{}};});
        require(r.outcome==GameModeLeafOutcome::nested_failed&&site==0x420ebe&&f.s.block_at_3b290c4.read(0x3b290c4,1)==1u,"release walk is a nested owner; the count is untouched");
        require(run_game_mode_leaf(0x420e90,0,f.s,ok).outcome==GameModeLeafOutcome::completed&&f.s.block_at_3b290c4.read(0x3b290c4,1)==0u,"count decremented after the release");}
    {Fixture f;f.mode.level_word_via_3b0ef4c=3;f.event=nullptr;require(run_game_mode_leaf(0x413300,{},f.s,ok).outcome==GameModeLeafOutcome::completed,"413300");
        require(!f.mode.level_word_via_3b0ef4c&&f.s.block_at_3b18250.read(0x3b18a08,4)==0x11111111u&&f.s.block_at_3b18250.read(0x3b18849,1)==1u&&f.s.byte_at_3abe134==1,"413300 hole, 3b18849 and the null level pointer");}
    {Fixture f;std::uint32_t calls=0;const auto r=run_game_mode_leaf(0x415000,{},f.s,[&](const GameModeLeafCall& c){++calls;return GameLogicOwnerResult{GameLogicOwnerState::completed,c.callee,{}};});
        require(r.outcome==GameModeLeafOutcome::completed&&calls==2&&f.s.byte_at_3abe134==0&&f.s.byte_at_3b0ef34==4&&f.rng.seed_at_2bcbdf8==0xdead0cabu&&f.game_mode==0x15&&f.s.callback_at_3d08420==0x40a340u,"415000 through 413c00 (785330 nested) to srand");}
    {Fixture f;for(auto [stage,eax]:{std::pair{25,2u},{26,1u},{34,1u},{35,2u},{38,2u},{39,1u},{-1,2u}}){f.stage=static_cast<std::uint16_t>(stage);require(run_game_mode_leaf(0x413c00,{},f.s,ok).eax==eax,"413c00 signed stage ranges");}}
    {Fixture f;f.timing.word_at_3b2a2e4=1;const auto r=run_game_mode_leaf(0x42c7b0,{},f.s,[](const GameModeLeafCall& c){return GameLogicOwnerResult{c.callee==0x42d3b0?GameLogicOwnerState::unavailable:GameLogicOwnerState::completed,c.callee,{}};});
        require(r.outcome==GameModeLeafOutcome::nested_failed&&r.source_boundary==0x42c7cb&&f.timing.word_at_3b2a2e4==0&&f.cutscene==0,"42c7b0 clears the borrowed demo words before 42d3b0");}
    std::cout<<"Game-mode leaves: unknown facts, borrowed writes, character bound, nested release walk, 413300 hole, 415000 and 42c7b0 nested stops passed.\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
