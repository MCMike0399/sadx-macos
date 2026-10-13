#include "game/game_startup.hpp"
#include <iostream>
#include <stdexcept>
#include <vector>
namespace {
void require(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
struct Fixture {
    sadx::GameLogicEdgeState edges;std::optional<std::int32_t> save_phase;sadx::GameFrameInputState input{edges,{},{},save_phase};sadx::GameControllerInitGlobals g;sadx::GameLogicController tmpl;
    std::array<std::shared_ptr<sadx::GameLogicController>,4> raw;sadx::GameLogicControllers pointers;sadx::InputModRawRecords mod;
    sadx::AudioStartupState audio;sadx::MenuBlockState menu;sadx::ExitCallbackList exits;std::optional<std::int32_t> mode=7;std::optional<std::uint32_t> w80=1,w78=1,w90=1;
    std::vector<sadx::StartupCall> calls;
    sadx::StartupRuntime rt{{edges,input,g,tmpl,raw,pointers,nullptr,audio,menu,exits,mode,w80,w78,w90},{}};
    Fixture(){for(auto& r:raw)r=std::make_shared<sadx::GameLogicController>();for(auto& r:mod)r=std::make_shared<sadx::GameLogicController>();
        audio.byte_at_3b29ce8=3;menu.block_at_900e88[0]=0;for(auto& s:exits.slots_at_3d084b0)s=0;
        rt.invoke=[this](const sadx::StartupCall& c){calls.push_back(c);return sadx::GameLogicOwnerResult{sadx::GameLogicOwnerState::completed,0};};}
};
}
int main(){try{
    {Fixture f;auto r=sadx::run_startup_40c030(f.rt);
     require(r&&r.exit_callback_registered&&f.exits.slots_at_3d084b0[0]==0x410630u&&f.mode==0&&f.w90==0u&&f.audio.byte_at_3b29ce8==4&&f.calls.size()==6,"startup order and stores");
     require(f.edges.raw_at_3b0e3e2[254][2]==0&&!f.edges.raw_at_3b0e3e2[255][1]&&f.edges.bytes_at_3b0e3e0[1]==0,"40f350 clears exactly 3b0e3e0..3b0e6df");
     require(f.pointers[1]&&*f.pointers[1]==f.raw[1]&&f.menu.block_at_9008b4[3]==1,"vanilla pointers and 40a8a0 copy");}
    {Fixture f;f.rt.owners.input_mod_raw_input_at_1000e748=&f.mod;require(bool(sadx::run_startup_40c030(f.rt))&&*f.pointers[2]==f.mod[2],"Input mod hook repoints ControllerPointers");}
    {Fixture f;for(auto& s:f.exits.slots_at_3d084b0)s=1;auto r=sadx::run_startup_40c030(f.rt);require(r&&!r.exit_callback_registered,"full callback list returns 0");}
    {Fixture f;f.exits.slots_at_3d084b0[3].reset();f.exits.slots_at_3d084b0[0]=1;f.exits.slots_at_3d084b0[1]=1;f.exits.slots_at_3d084b0[2]=1;auto r=sadx::run_startup_40c030(f.rt);
     require(r.failure==sadx::StartupFailure::unknown_state&&r.source_boundary==0x789932,"unknown slot stops the search");}
    {Fixture f;f.audio.byte_at_3b29ce8.reset();auto r=sadx::run_startup_40c030(f.rt);require(r.failure==sadx::StartupFailure::unknown_state&&r.source_boundary==0x424c4c&&f.audio.word_at_3b29b7c==0u&&f.calls.size()==4,"unknown counter after 410510 stores");}
    {Fixture f;f.menu.block_at_900e88[0].reset();auto r=sadx::run_startup_40c030(f.rt);require(r.failure==sadx::StartupFailure::unknown_state&&r.source_boundary==0x40a8a0&&!f.menu.byte_at_3abdbc1,"unknown 900e88 before any 40a8a0 store");}
    {Fixture f;f.raw[2].reset();auto r=sadx::run_startup_40c030(f.rt);require(r.failure==sadx::StartupFailure::controller_record_missing&&f.mode==7,"missing controller record stops before the mode store");}
    std::cout<<"Game startup: 40c030 order, 40f350/40a8a0/424c20/789930 stores, Input mod repoint and unknown boundaries passed.\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
