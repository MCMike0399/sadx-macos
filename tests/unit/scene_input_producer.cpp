#include "player/scene_input_producer.hpp"
#include <iostream>
#include <stdexcept>
namespace {
void require(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
using sadx::GameLogicController;using sadx::SceneInputFailure;
std::shared_ptr<GameLogicController> raw_pad(std::uint32_t seed){
    auto pad=std::make_shared<GameLogicController>();
    for(std::size_t i=0;i<GameLogicController::byte_count;++i)pad->write_byte(i,static_cast<std::uint8_t>(seed+i));
    pad->write_u16(0x1c,0);pad->write_u16(0x1e,0);pad->write_u32(8,0);pad->write_u32(0x10,0);
    return pad;
}
struct Fixture {
    sadx::GameLogicControllers raw;
    sadx::GameLogicEnginePads pads;
    sadx::GameControllerButtonMasks masks;
    sadx::SceneDirectionalKeyState keys;
    std::optional<std::uint16_t> stage=0x1a;
    std::optional<std::uint8_t> control=1;
    std::array<std::optional<std::uint8_t>,4> enabled{{1,1,1,1}};
    sadx::SceneInputProducerState state{raw,pads,masks,keys,stage,control,[&](std::size_t i)->std::optional<std::uint8_t>{return enabled[i];}};
    unsigned analog_calls=0;
    sadx::SceneInputProducerRuntime runtime{state,[this](sadx::X87Context&){++analog_calls;return sadx::PlayerInputProducerResult{};}};
    sadx::X87Context fp{0x27f};
    Fixture(){
        for(unsigned i=0;i<4;++i)raw[i]=raw_pad(0x40*i);
        for(auto& pad:pads.records)for(std::size_t i=0;i<GameLogicController::byte_count;++i)pad.write_byte(i,0xee);
        for(unsigned i=0;i<4;++i){keys.held_work_at_3b0e3a8[i]=0;keys.pressed_work_at_3b0e354[i]=0;keys.repeat_at_3b0e3bc[i]=0;}
    }
    sadx::SceneInputResult run(){return sadx::produce_scene_input(runtime,fp);}
};
bool cleared(const GameLogicController& pad,std::optional<std::uint32_t> old){
    return pad.read_u32(0x2c)==old&&pad.read_u32(8)==0&&pad.read_u32(0xc)==0xffffffffU&&pad.read_u32(0x10)==0&&pad.read_u32(0x14)==0&&
        pad.read_u32(0x18)==0&&pad.read_u32(0x1c)==0&&pad.read_u32(0x20)==0&&pad.read_u32(0)==0xeeeeeeeeU&&pad.read_u32(0x24)==0xeeeeeeeeU&&pad.read_u32(0x30)==0xeeeeeeeeU;
}
void tests(){
    require(!sadx::scene_input_hub_stage(0x26)&&sadx::scene_input_hub_stage(0x27)&&sadx::scene_input_hub_stage(0x2a)&&!sadx::scene_input_hub_stage(0x2b)&&!sadx::scene_input_hub_stage(0xff27),"4146b0 classifies exactly stages27..2a by full16-bit word");
    {Fixture f;f.raw[0].value()->forget(0x31,1);auto r=f.run();
     require(r&&f.masks.mask_at_909f94==0x402&&f.masks.mask_at_909fa4==0x402&&!f.masks.mask_at_909f98,"Non-hub stage stores 402 into both owned masks only");
     require(r.copied_words==26&&r.cleared_records==6&&f.analog_calls==1,"Slots0/1 copy13 dwords each; remaining six records clear");
     for(std::size_t i=0;i<GameLogicController::byte_count;++i)require(f.pads.records[0].read_byte(i)==f.raw[0].value()->read_byte(i),"rep movsd copies raw bytes including unknown facts");
     require(!f.pads.records[0].read_byte(0x31),"Unknown raw byte stays unknown after copy");
     for(std::size_t slot=2;slot<8;++slot)require(cleared(f.pads.records[slot],0xeeeeeeeeU),"Clear copies previous held into+2c then performs source stores");}
    {Fixture f;f.stage=0x29;auto r=f.run();require(r&&f.masks.mask_at_909f94==2&&f.masks.mask_at_909fa4==0x402,"Hub stage stores2 then402");}
    {Fixture f;f.enabled[0]=2;f.enabled[1]=0;f.raw[0]=std::shared_ptr<GameLogicController>{};f.raw[1]=std::shared_ptr<GameLogicController>{};auto r=f.run();
     require(r&&r.copied_words==0&&r.cleared_records==8&&cleared(f.pads.records[0],0xeeeeeeeeU),"Controller enable requires ==1; other bytes clear without dereferencing null raw pointers");}
    {Fixture f;f.control=2;f.raw[0].value()->write_u32(8,0x400);f.raw[0].value()->write_u32(0x10,0x400);auto r=f.run();
     require(r&&r.cleared_records==8,"Control byte2 is not the copy branch (40fdf7 tests ==1)");
     require(f.keys.held_out_at_3b0e7e0[0]==0x400&&f.keys.pressed_out_at_3b0e3cc[0]==0x400,"40f070 output gate is !=0, distinct from40fdc0 ==1");}
    {Fixture f;f.control=0;f.raw[0].value()->write_u32(8,0x400);f.keys.held_out_at_3b0e7e0[1]=7;auto r=f.run();
     require(r&&r.cleared_records==8&&r.copied_words==0,"Disabled control clears all eight records");
     require(f.keys.held_out_at_3b0e7e0[1]==0&&f.keys.pressed_out_at_3b0e3cc[1]==0&&f.keys.held_out_at_3b0e7e0[0]==0&&f.keys.held_work_at_3b0e3a8[0]==0x400,"Disabled control zeroes outputs while work still updates");}
    {Fixture f;f.raw[2].reset();auto r=f.run();
     require(r.failure==SceneInputFailure::directional_failed&&r.slot==2&&r.nested_source_boundary==0x40f074&&r.directional_failure==SceneInputFailure::unknown_controller_reference,"Unknown raw pointer word in40f070 is a boundary");}
    {Fixture f;f.stage.reset();auto r=f.run();require(r.failure==SceneInputFailure::unknown_stage&&r.nested_source_boundary==0x4146b0&&!f.masks.mask_at_909f94&&f.analog_calls==0,"Unknown stage stops before mask stores");}
    {Fixture f;f.control.reset();auto r=f.run();
     require(r.failure==SceneInputFailure::unknown_control_enabled&&r.source_boundary==0x40fdef&&f.masks.mask_at_909f94==0x402&&f.pads.records[0].read_u32(8)==0xeeeeeeeeU,"Mask stores precede the unknown control read; pads untouched");}
    {Fixture f;f.raw[1]=std::shared_ptr<GameLogicController>{};auto r=f.run();
     require(r.failure==SceneInputFailure::null_controller&&r.source_boundary==0x40fe46&&r.slot==1&&r.copied_words==13,"Null slot1 pointer faults at its copy after slot0 completed");
     require(f.pads.records[0].read_u32(0x10)==0&&f.pads.records[2].read_u32(8)==0xeeeeeeeeU&&f.analog_calls==0,"Partial copy retained; later records and owners untouched");}
    {Fixture f;f.enabled[1].reset();auto r=f.run();require(r.failure==SceneInputFailure::unknown_controller_enabled&&r.source_boundary==0x40fe2d,"Unknown909fb5 stops at its compare");}
    {Fixture f;f.runtime.analog_owner={};auto r=f.run();
     require(r.failure==SceneInputFailure::missing_analog_owner&&r.nested_source_boundary==0x40f170&&r.copied_words==26&&!f.keys.held_out_at_3b0e7e0[0],"Missing40f170 owner stops after pad stores and before40f070");}
    {Fixture f;f.runtime.analog_owner=[&](sadx::X87Context&){require(f.pads.records[0].read_u32(0x10)==0,"40f170 observes completed pad copies");
        return sadx::PlayerInputProducerResult{sadx::PlayerInputProducerFailure::unavailable_camera_yaw,0x40f29c,1};};
     auto r=f.run();require(r.failure==SceneInputFailure::analog_producer_failed&&r.nested_source_boundary==0x40f29c&&r.analogs&&r.analogs->slot==1&&!f.keys.held_out_at_3b0e7e0[0],"Analog failure keeps nested result");}
    {Fixture f;auto& pad=*f.raw[2].value();pad.write_u16(0x1e,79);pad.write_u16(0x1c,static_cast<std::uint16_t>(-78));
     f.keys.repeat_at_3b0e3bc[2]=30;auto r=f.run();
     require(r&&f.keys.held_work_at_3b0e3a8[2]==0x20&&f.keys.pressed_work_at_3b0e354[2]==0x20&&f.keys.repeat_at_3b0e3bc[2]==0,"Strict +-78 thresholds; new press resets repeat");
     r=f.run();require(r&&f.keys.pressed_work_at_3b0e354[2]==0&&f.keys.repeat_at_3b0e3bc[2]==1,"Held direction increments repeat");
     f.keys.repeat_at_3b0e3bc[2]=30;r=f.run();require(r&&f.keys.repeat_at_3b0e3bc[2]==25&&f.keys.pressed_work_at_3b0e354[2]==0x20&&f.keys.pressed_out_at_3b0e3cc[2]==0x20,"Repeat >30 subtracts6 and re-presses held directions");}
    {Fixture f;f.raw[3]=std::shared_ptr<GameLogicController>{};f.keys.held_out_at_3b0e7e0[3]=0x1234;auto r=f.run();
     require(r&&f.keys.held_out_at_3b0e7e0[3]==0x1234,"Observed null raw pointer leaves40f070 work/output untouched");}
    {Fixture f;f.keys.held_work_at_3b0e3a8[2].reset();auto r=f.run();
     require(r.failure==SceneInputFailure::directional_failed&&r.slot==2&&r.nested_source_boundary==0x40f0b9&&r.directional_failure==SceneInputFailure::unknown_held_work&&f.keys.held_out_at_3b0e7e0[1].has_value()&&!f.keys.held_out_at_3b0e7e0[3],"Directional failure keeps earlier slots and stops later ones");}
    {sadx::GameLogicController source;source.write_u32(8,0x55);source.forget(0x30,1);std::array<std::shared_ptr<sadx::GameLogicController>,4> raw;for(auto& r:raw)r=std::make_shared<sadx::GameLogicController>();
     sadx::GameLogicControllers pointers;sadx::GameControllerInitGlobals g;require(sadx::init_controllers_40f3a0(source,raw,pointers,g),"40f3a0 initializes");
     require(raw[3]->read_u32(8)==0x55u&&!raw[3]->read_byte(0x30)&&pointers[3]&&*pointers[3]==raw[3]&&g.bytes_at_3b0e9a4[2]==0&&g.words_at_3b0e990[4]==0u,"template replicated to all four records, pointers to ControllersRaw");
     raw[1].reset();sadx::GameControllerInitGlobals h;require(!sadx::init_controllers_40f3a0(source,raw,pointers,h)&&h.words_at_3b0e768[3]==0u&&!h.words_at_3b0e344[0],"missing record allocation stops after the first stores");}
}
}
int main(){try{tests();std::cout<<"Scene input: 4146b0 masks, ==1 copy/clear gates, partial raw facts, 40f170 order and 40f070 repeat/null boundaries passed.\n";return 0;}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
