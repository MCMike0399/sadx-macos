#include "input/input_mod_pad.hpp"
#include "player/scene_input_producer.hpp"
#include "player/sonic_player.hpp"
#include <iostream>
#include <stdexcept>
// Keyboard events -> InputState (Input 2.6.1 ramp) -> Input mod OnInput ->
// raw_input via ControllerPointers -> 40fdc0 engine pads -> SonicPlayerControls.
// Composition of native owners only; no physical keyboard or window.
namespace {
void require(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
const sadx::InputModAxesConstants axes{{0x34000000},{0x46fffe00},{0x42fe0000},{0x43000000},{0x3f800000}};
const sadx::InputModPadConstants pad_constants{axes,{0x437f0000},{0x3f000000}};
}
int main(){try{
    sadx::input::InputState keyboard;sadx::SseContext sse(0x1f80);
    std::array<sadx::InputModDreamPad,8> pads{};
    for(auto& p:pads){for(unsigned w=0;w<13;++w)p.record.write_u32(w*4,0);p.settings.trigger_threshold=30;p.settings.deadzone_l=7849;p.settings.deadzone_r=8689;p.settings.radial_l=true;}
    pads[0].settings.allow_keyboard=true; // profile: keyboardPlayer 0
    std::array<std::shared_ptr<sadx::GameLogicController>,4> controllers_raw;for(auto& r:controllers_raw)r=std::make_shared<sadx::GameLogicController>();
    sadx::InputModRawRecords raw_input;for(auto& r:raw_input)r=std::make_shared<sadx::GameLogicController>();
    sadx::GameLogicControllers pointers;sadx::GameControllerInitGlobals init;sadx::GameLogicController template_record;for(unsigned w=0;w<13;++w)template_record.write_u32(w*4,0);
    require(sadx::init_controllers_40f3a0(template_record,controllers_raw,pointers,init),"vanilla InitControllers");
    sadx::input_mod_init_raw_controllers(raw_input,pointers); // Input mod RET hook at 40f451
    sadx::InputModFrameOwners owners{pads,controllers_raw,raw_input,pointers};
    std::array<sadx::InputModGamepadState,8> no_gamepads{};std::optional<std::int32_t> mode=4,demo=0;sadx::InputModFrameState frame;frame.game_mode_at_3abdc7c=&mode;frame.demo_playing_at_3b2a2e4=&demo;
    sadx::GameLogicEnginePads engine;sadx::GameControllerButtonMasks masks;sadx::SceneDirectionalKeyState keys;
    for(unsigned i=0;i<4;++i){keys.held_work_at_3b0e3a8[i]=0;keys.pressed_work_at_3b0e354[i]=0;keys.repeat_at_3b0e3bc[i]=0;}
    for(auto& r:engine.records)for(unsigned w=0;w<13;++w)r.write_u32(w*4,0);
    std::optional<std::uint16_t> stage=0x1a;std::optional<std::uint8_t> control=1;
    // Profile: 40fe0d/40fe2f read the Input mod's controller_enabled (1000e73c), not 909fb4.
    sadx::InputModControllerTable controller_table;controller_table.enabled_at_1000e73c.fill(1);
    sadx::SceneInputProducerState scene{pointers,engine,masks,keys,stage,control,[&](std::size_t i)->std::optional<std::uint8_t>{return controller_table.enabled_at_1000e73c[i];}};
    sadx::SceneInputProducerRuntime scene_runtime{scene,[](sadx::X87Context&){return sadx::PlayerInputProducerResult{};}};
    sadx::X87Context fp(0x7f);
    const auto frame_tick=[&](){
        const auto tick=keyboard.sample_tick(sse,axes);
        const auto r=sadx::input_mod_on_input(owners,sadx::input_mod_keyboard_pad(tick),no_gamepads,frame,pad_constants,sse);require(bool(r),"Input mod OnInput");
        const auto s=sadx::produce_scene_input(scene_runtime,fp);require(bool(s),"40fdc0 scene input");
    };
    // Return = Start, X = jump (A) with the default bindings; Right arrow ramps the stick.
    keyboard.keyboard_event(40,true);keyboard.keyboard_event(27,true);keyboard.keyboard_event(79,true);frame_tick();
    require(raw_input[0]->read_u32(8)==(0x8u|0x4u)&&controllers_raw[0]->read_u32(8)==(0x8u|0x4u),"keyboard buttons reach raw_input and ControllersRaw");
    require(engine.records[0].read_u32(8)==(0x8u|0x4u)&&engine.records[0].read_u32(0x10)==(0x8u|0x4u),"40fdc0 copies held/pressed into engine pad 0");
    require(engine.records[0].read_u16(0x1c)==127,"first ramp step: Input mod normalized direction 127");
    sadx::SonicPlayerControls controls{&engine,&masks};
    require(controls.masks->mask_at_909f94==0x402u,"40fdc0 wrote the non-hub action mask");
    frame_tick();require(engine.records[0].read_u32(0x10)==0u&&engine.records[0].read_u32(8)==(0x8u|0x4u),"held without new press on the next frame");
    keyboard.keyboard_event(27,false);frame_tick();require(engine.records[0].read_u32(0x14)==0x4u,"jump release edge");
    std::cout<<"Input chain: keyboard -> Input 2.6.1 OnInput -> ControllerPointers(raw_input) -> 40fdc0 engine pads passed.\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
