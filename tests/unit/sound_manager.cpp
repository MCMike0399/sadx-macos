#include "game/sound_manager.hpp"
#include <iostream>
#include <stdexcept>
// Semantics the oracle does not compare: unknown facts, owner failures and the
// survey continuation when a completed helper supplies no return value.
namespace {
using namespace sadx;
void require(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
struct Fixture {
    AudioStartupState audio;std::array<std::optional<std::uint32_t>,36> flags{},entities{};SoundRecordFields fields;std::vector<std::uint32_t> calls;
    SoundManagerRuntime rt{audio,flags,entities,fields,{}};
    Fixture(){audio.word_at_3b29ce0=0;audio.word_at_3b29ce4=0;audio.byte_at_3b29ce8=0;for(auto& w:audio.record_words_at_3b292fc)w=0;for(auto& w:audio.words_at_3b292a8)w=7;
        for(auto& f:flags)f=0;for(auto& e:entities)e=5;fields.listener_at_3b2cbb0=0;for(auto& a:fields.ids_at_3b29308)a=3;for(auto& a:fields.words_at_3b29300)a=9;for(auto& a:fields.current_volume_at_3b29c28)a=0;
        rt.invoke=[this](const SoundManagerCall& c){calls.push_back(c.callee);return SoundManagerOwnerResult{{GameLogicOwnerState::completed,0,{}},std::nullopt};};}
};
}
int main(){try{
    {Fixture f;require(bool(run_sound_manager_4250d0(f.rt))&&f.fields.ids_at_3b29308[35]==0xffffffffu&&f.entities[0]==0u&&f.fields.words_at_3b29300[7]==0u&&f.calls.empty()&&f.audio.byte_at_3b29ce8==1,"idle records expire and clear");}
    {Fixture f;f.audio.byte_at_3b29ce8=4;require(bool(run_sound_manager_4250d0(f.rt))&&f.audio.byte_at_3b29ce8==0&&f.audio.words_at_3b292a8[17]==0u,"every fifth frame clears 3b292a8");}
    {Fixture f;f.flags[2]=1;f.fields.ids_at_3b29308[2]=0x100;f.audio.record_words_at_3b292fc[2]=4;
        require(bool(run_sound_manager_4250d0(f.rt))&&f.calls==std::vector<std::uint32_t>{0x4246c0}&&f.audio.record_words_at_3b292fc[2]==4u,"an active record whose helper returns nothing is skipped (survey)");}
    {Fixture f;f.flags[1]=0x11;f.fields.ids_at_3b29308[1]=0x600;require(bool(run_sound_manager_4250d0(f.rt))&&f.calls==std::vector<std::uint32_t>{0x425710}&&f.flags[1]==0u,"high ids stop through 425710 and clear 0x11");}
    {Fixture f;f.audio.record_words_at_3b292fc[0]=2;f.flags[0]=0x800;f.fields.frequency_at_3b2930c[0]=1000;std::array<std::optional<std::uint32_t>,2> args{};
        f.rt.invoke=[&](const SoundManagerCall& c){args=c.arguments;return SoundManagerOwnerResult{{GameLogicOwnerState::completed,0,{}},std::nullopt};};
        require(bool(run_sound_manager_4250d0(f.rt))&&args[1]==10000u&&f.flags[0]==0u&&f.audio.record_words_at_3b292fc[0]==1u,"frequency clamp, flag clear, timer step");}
    {Fixture f;f.fields.ids_at_3b29308[0].reset();f.flags[0]=1;const auto r=run_sound_manager_4250d0(f.rt);require(r.failure==SoundManagerFailure::unknown_state&&r.required_fact==0x3b29308u,"unknown id");}
    {Fixture f;f.fields.listener_at_3b2cbb0=0x1234;f.rt.invoke=[](const SoundManagerCall& c){return SoundManagerOwnerResult{{GameLogicOwnerState::unavailable,c.callee,{}},std::nullopt};};
        const auto r=run_sound_manager_4250d0(f.rt);require(r.failure==SoundManagerFailure::missing_owner&&r.source_boundary==0x4253b1&&r.pending_call&&r.pending_call->callee==0x424b10,"a listener reaches the 424b10 owner");}
    std::cout<<"Sound manager: idle expiry, 3b292a8 clear, active-record owners, frequency clamp, unknown facts and the 424b10 listener boundary passed.\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
