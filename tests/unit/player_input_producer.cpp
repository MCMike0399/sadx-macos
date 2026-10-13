#include "player/player_input_producer.hpp"
#include "input/input.hpp"

#include <cstring>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <vector>

namespace {
void require(bool condition,const char* message) { if(!condition) throw std::runtime_error(message); }
std::uint32_t word(float v) { std::uint32_t w;std::memcpy(&w,&v,4);return w; }
float value(std::uint32_t w) { float v;std::memcpy(&v,&w,4);return v; }
sadx::GameLogicControllers raw_pads; // borrowed by every runtime below
void set_axes(sadx::PlayerInputProducerRuntime& r,unsigned slot,std::int16_t x,std::int16_t y) {
    auto pad=std::make_shared<sadx::GameLogicController>();
    pad->write_u16(0x1c,static_cast<std::uint16_t>(x));pad->write_u16(0x1e,static_cast<std::uint16_t>(y));
    raw_pads[slot]=pad;r.raw_controllers=&raw_pads;
}
sadx::PlayerInputProducerRuntime explicit_runtime() {
    sadx::PlayerInputProducerRuntime r;
    set_axes(r,0,0,0);set_axes(r,1,0,0);
    r.camera=sadx::PlayerInputCamera{false,{}};r.control_enabled=1;
    for(auto& enabled:r.controller_enabled) enabled=0;
    r.controller_enabled[0]=1;
    return r;
}
}

int main(int argc,char** argv) {
    using namespace sadx;
    SseContext sse(0x1f80); // Test-selected MXCSR, not observed PC state.
    const InputModAxesConstants axes_constants{{0x34000000}, {0x46fffe00},
        {0x42fe0000}, {0x43000000}, {0x3f800000}}; // Pinned DLL .rdata.
    require(argc==2,"supply extracted source input constants");
    std::ifstream file(argv[1],std::ios::binary);
    std::vector<std::uint8_t> bytes{std::istreambuf_iterator<char>(file),{}};
    const auto parsed=decode_player_input_producer_constants(bytes.data(),bytes.size());
    require(parsed.has_value(),"source constants decoded");const auto& c=*parsed;
    require(c.positive_deadzone==3072 && c.negative_deadzone==-3072 && c.magnitude_cap==1,
            "binary source deadzone/cap identity");
    require(!decode_player_input_producer_constants(nullptr,40) &&
            !decode_player_input_producer_constants(bytes.data(),39),"malformed constants rejected");
    X87Context fp(0x037f);
    auto runtime=explicit_runtime();PlayerInputProducedState state;
    set_axes(runtime,0,12,-12);
    require(static_cast<bool>(write_analogs_bochs_p5_p6(runtime,c,state,fp)),"deadzone-edge producer");
    for(unsigned i=0;i<8;i++) require(state.raw_unused[i] && state.normalized[i] &&
        state.raw_unused[i]->angle==0 && word(state.raw_unused[i]->magnitude)==0 &&
        state.normalized[i]->angle==0 && word(state.normalized[i]->magnitude)==0,
        "deadzone inclusive edges and higher slots must store positive zeros");
    set_axes(runtime,0,127,0);
    require(static_cast<bool>(write_analogs_bochs_p5_p6(runtime,c,state,fp)),"full X axis");
    require(state.normalized[0]->angle==0 && state.normalized[0]->magnitude>0.99F,
            "original cubic magnitude/full cardinal direction");
    set_axes(runtime,0,13,0);
    write_analogs_bochs_p5_p6(runtime,c,state,fp);
    require(state.normalized[0]->magnitude>0 && state.normalized[0]->magnitude<0.000001F,
            "source magnitude must retain cube rather than linear analog strength");
    runtime.camera=PlayerInputCamera{true,0x12345};
    write_analogs_bochs_p5_p6(runtime,c,state,fp);
    require(state.normalized[0]->angle==0xdcbb,"camera yaw negation must mask to 16 bits");
    runtime.camera=PlayerInputCamera{true,{}};
    require(write_analogs_bochs_p5_p6(runtime,c,state,fp).failure==PlayerInputProducerFailure::unavailable_camera_yaw,
            "missing present-camera yaw must not become identity");
    runtime.camera.reset();
    require(write_analogs_bochs_p5_p6(runtime,c,state,fp).failure==PlayerInputProducerFailure::unavailable_camera_presence,
            "unknown camera must differ from observed null pointer");

    runtime=explicit_runtime();set_axes(runtime,0,127,-127);
    runtime.controller_enabled[1].reset();state.normalized[1]=NormalizedPlayerInput{999,77};
    auto result=write_analogs_bochs_p5_p6(runtime,c,state,fp);
    require(result.failure==PlayerInputProducerFailure::unavailable_controller_enabled && result.slot==1 &&
            state.raw_unused[1]->angle==0 && state.normalized[1]->angle==999 && state.normalized[0]->magnitude==1,
            "partial slot order/raw-before-gate writes must survive later failure");
    runtime=explicit_runtime();runtime.control_enabled=0;runtime.controller_enabled[0].reset();
    require(static_cast<bool>(write_analogs_bochs_p5_p6(runtime,c,state,fp)),
            "disabled global control must not require a skipped controller gate");
    runtime=explicit_runtime();set_axes(runtime,0,127,-127);raw_pads[1]=std::shared_ptr<GameLogicController>{};
    state.normalized[1]=NormalizedPlayerInput{999,77};result=write_analogs_bochs_p5_p6(runtime,c,state,fp);
    require(result.failure==PlayerInputProducerFailure::null_raw_controller && result.source_boundary==0x40f18c && result.slot==1 &&
            state.normalized[0]->magnitude==1 && state.normalized[1]->angle==999,"observed null slot1 pointer faults at its axis read after slot0");
    set_axes(runtime,1,5,5);(*raw_pads[1])->forget(0x1f,1);result=write_analogs_bochs_p5_p6(runtime,c,state,fp);
    require(result.failure==PlayerInputProducerFailure::unavailable_raw_axis && result.source_boundary==0x40f190 && result.slot==1,
            "unknown Y byte stops at the second signed16 load");
    raw_pads[1].reset();result=write_analogs_bochs_p5_p6(runtime,c,state,fp);
    require(result.failure==PlayerInputProducerFailure::unavailable_raw_controller && result.source_boundary==0x40f185,
            "unknown pointer word differs from observed null");

    runtime=explicit_runtime();runtime.input_mod_pads[0]=InputMod261Pad{0,1,PlayerInputPadAxes{13,0},0.25F};
    set_axes(runtime,0,13,0);
    require(static_cast<bool>(produce_input_mod_261_bochs_p5_p6(runtime,c,state,fp)) &&
            word(state.normalized[0]->magnitude)==word(0.25F),"updated hook must override exact prepared strength");
    const auto direction=state.normalized[0]->angle;
    runtime.input_mod_pads[0]->axes=PlayerInputPadAxes{12,-12};
    runtime.input_mod_pads[0]->normalized_magnitude=0.8F;
    require(static_cast<bool>(override_input_mod_261_magnitudes(runtime,state)) &&
            state.normalized[0]->angle==direction && word(state.normalized[0]->magnitude)==word(0.25F),
            "updated threshold is strict and does not recompute direction");
    runtime.controller_enabled[2]=1;
    runtime.input_mod_pads[2]=InputMod261Pad{0,1,{}, {}};
    require(static_cast<bool>(override_input_mod_261_magnitudes(runtime,state)),
            "disconnected keyboard allowance on slot2 must skip axes/magnitude");
    runtime.input_mod_pads[2]=InputMod261Pad{1,{},PlayerInputPadAxes{-32768,0},value(0x7f812345)};
    state.normalized[2].reset();
    const auto old_flags=fp.exception_flags();
    require(static_cast<bool>(override_input_mod_261_magnitudes(runtime,state)) &&
            !state.normalized[2]->angle && word(state.normalized[2]->magnitude)==0x7f812345 && fp.exception_flags()==old_flags,
            "MOV hook must preserve unknown angle/SNaN bits without x87 math");
    runtime.control_enabled=2;runtime.controller_enabled[0]=2;
    runtime.input_mod_pads[0]=InputMod261Pad{1,{},PlayerInputPadAxes{127,0},0.5F};
    require(static_cast<bool>(produce_input_mod_261_bochs_p5_p6(runtime,c,state,fp)) &&
            state.normalized[0]->angle==0 && state.normalized[0]->magnitude==0.5F,
            "original ==1 and hook !=0 byte tests must remain distinct");

    // Real shared keyboard events/compatibility ramp into the new producer.
    input::InputState keys;keys.keyboard_event(79,true);const auto tick=keys.sample_tick(sse, axes_constants);
    runtime=explicit_runtime();set_axes(runtime,0,tick.left.x,tick.left.y);
    runtime.input_mod_pads[0]=InputMod261Pad{0,1,PlayerInputPadAxes{tick.left.x,tick.left.y},tick.left.magnitude};
    require(static_cast<bool>(produce_input_mod_261_bochs_p5_p6(runtime,c,state,fp)),"keyboard frame into producer");
    PlayerInputRuntime query_runtime;query_runtime.stage=1;query_runtime.no_control_timer[0]=0;
    query_runtime.normalized=state.normalized;
    std::int32_t angle=777;float magnitude=77;
    const auto query=get_player_input(PlayerInputTask{0,0,0,0,{}},query_runtime,fp,&angle,&magnitude,{});
    require(query && query.active==true && angle==0 && word(magnitude)==word(tick.left.magnitude),
            "source producer must carry real keyboard ramp magnitude into GetAnalog");
    std::cout<<"player input producer passed\n";
}
