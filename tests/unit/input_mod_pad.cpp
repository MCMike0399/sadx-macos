#include "input/input_mod_pad.hpp"
#include <iostream>
#include <stdexcept>
namespace {
void require(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
sadx::InputModDreamPad pad(){sadx::InputModDreamPad p;for(unsigned w=0;w<13;++w)p.record.write_u32(w*4,0);p.settings.allow_keyboard=true;p.settings.trigger_threshold=30;p.settings.deadzone_l=7849;p.settings.deadzone_r=8689;p.settings.radial_l=true;return p;}
// Pinned DLL .rdata values (1000bb6c,bbb0,bba0,bba4,bb7c / bba8 / bb78).
const sadx::InputModPadConstants k{{{0x34000000},{0x46fffe00},{0x42fe0000},{0x43000000},{0x3f800000}},{0x437f0000},{0x3f000000}};
void tests(){
    sadx::SseContext sse(0x1f80);sadx::InputModGamepadState gp;sadx::InputModKeyboardPad kb;
    {auto p=pad();p.settings.allow_keyboard=false;const auto before=p.record.read_u32(8);auto r=sadx::input_mod_poll(p,kb,gp,false,k,sse);require(r&&!r.polled&&p.record.read_u32(8)==before&&!p.normalized_l,"disconnected without keyboard returns untouched");}
    {auto p=pad();kb.held=0x8;kb.lx=127;kb.ltrigger=255;auto r=sadx::input_mod_poll(p,kb,gp,false,k,sse);
     require(r&&p.record.read_u32(8)==(0x8u|0x20000u)&&p.record.read_u32(0x10)==(0x8u|0x20000u)&&p.record.read_u16(0x1c)==127&&p.record.read_u16(0x1a)==255,"keyboard-only pad: Start, L trigger edge, keyboard stick");
     r=sadx::input_mod_poll(p,kb,gp,false,k,sse);require(r&&p.record.read_u32(0x10)==0u&&p.record.read_u32(0x14)==0u&&p.record.read_u32(0x2c)==(0x8u|0x20000u),"held without new edges");
     kb=sadx::InputModKeyboardPad{};r=sadx::input_mod_poll(p,kb,gp,false,k,sse);require(r&&p.record.read_u32(0x14)==(0x8u|0x20000u)&&p.record.read_u32(0x0c)==0xffffffffu,"release edges");}
    {auto p=pad();sadx::input_mod_connect(p);require(p.record.read_u32(4)==0x3f0fffu,"connect sets Support");
     gp.axes={{32767,0,0,0,32767,0}};gp.buttons[0]=true;gp.buttons[8]=true;auto r=sadx::input_mod_poll(p,kb,gp,false,k,sse);
     require(r&&p.half_press&&p.record.read_u16(0x1c)==63&&(p.record.read_u32(8).value()&0x20004u)==0x20004u,"gamepad A, L trigger axis, right-stick half press halves 127 to 63");
     gp.buttons[8]=false;p.connected=false;r=sadx::input_mod_poll(p,kb,gp,false,k,sse);require(r&&p.half_press,"half press persists while disconnected (not re-read)");}
    {auto p=pad();p.record.forget(0x2e,1);auto r=sadx::input_mod_poll(p,kb,gp,false,k,sse);require(r.failure==sadx::InputModFailure::unknown_old_buttons&&r.source_boundary==0x100043d3&&p.record.read_u32(8)==0u,"Old is loaded before the Held store");}
    {auto p=pad();kb.held=0xffff;auto r=sadx::input_mod_poll(p,kb,gp,true,k,sse);require(r&&p.record.read_u32(8)==8u&&p.record.read_u32(0x1c)==0u,"demo keeps only Start and zeroes axes");kb=sadx::InputModKeyboardPad{};}
    {std::array<sadx::InputModDreamPad,8> pads{};for(auto& p:pads)p.record.write_u32(8,0x1234);std::array<std::shared_ptr<sadx::GameLogicController>,4> raw{};for(auto& r:raw)r=std::make_shared<sadx::GameLogicController>();
     sadx::InputModRawRecords mirror{};for(auto& m:mirror)m=std::make_shared<sadx::GameLogicController>();auto r=sadx::input_mod_publish(pads,raw,mirror);
     require(r&&raw[3]->read_u32(8)==0x1234u&&!raw[3]->read_u32(0)&&mirror[7]->read_u32(8)==0x1234u,"publish copies bytes, unknown stays unknown");
     sadx::GameLogicControllers pointers;sadx::input_mod_init_raw_controllers(mirror,pointers);require(pointers[2]&&*pointers[2]==mirror[2],"ControllerPointers repointed at the mod's raw_input");
     raw[2].reset();r=sadx::input_mod_publish(pads,raw,mirror);require(r.failure==sadx::InputModFailure::null_raw_controller&&r.source_boundary==0x10004919&&raw[1]->read_u32(8)==0x1234u,"missing raw record stops after earlier copies");}
    {sadx::InputModControllerTable t;sadx::InputModKeyboardPad kbp;kbp.normalized_r={0x3f800000};
     require(sadx::input_mod_enable_controller(2,t,nullptr)==sadx::InputModTableFailure::none&&t.enabled_at_1000e73c[0]==1&&t.enabled_at_1000e73c[1]==1&&t.enabled_at_1000e73c[2]==1&&t.enabled_at_1000e73c[3]==0,"index>=2 also enables 0 and 1");
     require(sadx::input_mod_enable_controller(8,t,nullptr)==sadx::InputModTableFailure::overflow_target_missing,"Enable(8) out-of-table write needs its real target");
     require(sadx::input_mod_enable_controller(8,t,&kbp)==sadx::InputModTableFailure::none&&kbp.normalized_r.bits==0x3f800001u,"Enable(8) writes the low byte of keyboard normalized_r");
     require(sadx::input_mod_disable_controller(9,t)==sadx::InputModTableFailure::none&&t.enabled_at_1000e73c==std::array<std::uint8_t,8>{},"index>8 disables all");
     require(sadx::input_mod_disable_controller(8,t)==sadx::InputModTableFailure::invalid_parameter_abort,"Disable(8) aborts");}
    {auto pointer=std::make_shared<sadx::GameLogicController>();pointer->write_u32(8,0x606);pointer->write_u32(0x14,8);sadx::GameLogicControllers pointers;pointers[0]=pointer;
     std::optional<std::int32_t> mode=4,demo=0;sadx::InputModFrameState s;s.game_mode_at_3abdc7c=&mode;s.demo_playing_at_3b2a2e4=&demo;auto r=sadx::input_mod_on_input_tail(pointers,s);require(r&&r.soft_reset&&s.soft_reset_byte==1,"soft reset combination");
     s.soft_reset_byte.reset();mode=8;r=sadx::input_mod_on_input_tail(pointers,s);require(r&&!r.soft_reset&&!s.soft_reset_byte,"no soft reset in mode 8");
     demo=1;s.demo_enabled=1;s.demo_cutscene_at_3b2a2e8=-1;r=sadx::input_mod_on_input_tail(pointers,s);require(r.failure==sadx::InputModFailure::demo_unported&&s.demo_at_1000e708,"demo playback stays an explicit boundary");
     pointers[0]=std::shared_ptr<sadx::GameLogicController>{};r=sadx::input_mod_on_input_tail(pointers,s);require(r.failure==sadx::InputModFailure::null_raw_controller&&r.source_boundary==0x10006520,"null pointer faults at the held compare");}
    {std::array<sadx::InputModDreamPad,8> pads{};for(auto& p:pads){p=pad();}std::array<std::shared_ptr<sadx::GameLogicController>,4> raw{};for(auto& r:raw)r=std::make_shared<sadx::GameLogicController>();
     sadx::InputModRawRecords raw_input{};for(auto& r:raw_input)r=std::make_shared<sadx::GameLogicController>();
     sadx::GameLogicControllers pointers;sadx::input_mod_init_raw_controllers(raw_input,pointers);sadx::InputModFrameOwners owners{pads,raw,raw_input,pointers};
     sadx::input::TickInput tick;tick.held=0x606;tick.left={127,0,0.25F};const auto kb=sadx::input_mod_keyboard_pad(tick);require(kb.lx==127&&kb.normalized_l.bits==0x3e800000u&&kb.ltrigger==0,"keyboard tick adapter");
     std::array<sadx::InputModGamepadState,8> gps{};std::optional<std::int32_t> state_mode=4,state_demo=0;sadx::InputModFrameState state;state.game_mode_at_3abdc7c=&state_mode;state.demo_playing_at_3b2a2e4=&state_demo;
     auto r=sadx::input_mod_on_input(owners,kb,gps,state,k,sse);require(r&&raw[0]->read_u32(8)==0x606u&&raw_input[0]->read_u32(8)==0x606u&&(*pointers[0])->read_u16(0x1c)==127&&!r.soft_reset,"frame publishes to raw_input (read via ControllerPointers) and ControllersRaw");
     tick.held=0x606;sadx::input::TickInput release=tick;r=sadx::input_mod_on_input(owners,sadx::input_mod_keyboard_pad(release),gps,state,k,sse);require(bool(r),"second frame");}
}
}
int main(){try{tests();std::cout<<"Input mod pad: DLL poll/convert/publish/soft-reset semantics and explicit boundaries passed.\n";return 0;}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
