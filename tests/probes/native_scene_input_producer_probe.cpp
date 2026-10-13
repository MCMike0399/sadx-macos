#include "input/scene_input_producer.hpp"
#include <algorithm>
// Fixture byte pattern shared with tools/verify_original_scene_input_producer.py.
namespace {std::uint8_t pattern(std::uint32_t seed,std::uint32_t tag,std::uint32_t index){return static_cast<std::uint8_t>(seed*31U+tag*17U+index*7U);}
std::uint32_t pattern32(std::uint32_t seed,std::uint32_t tag,std::uint32_t index){std::uint32_t v=0;for(std::uint32_t b=0;b<4;++b)v|=std::uint32_t(pattern(seed,tag,index*4+b))<<(8*b);return v;}}
// 40f170 is an explicit owner fixture; 4146b0 and 40f070 are native bodies.
extern "C" int sadx_native_scene_input_producer(const std::uint32_t* in,std::size_t size,std::uint32_t* out,std::size_t capacity){
    if(!in||size!=32||!out||capacity<130)return 0;
    const auto seed=in[6];
    sadx::GameLogicControllers raw;sadx::GameLogicEnginePads pads;sadx::GameControllerButtonMasks masks;sadx::SceneDirectionalKeyState keys;
    for(std::uint32_t i=0;i<4;++i){
        if(in[4]>>i&1U){raw[i]=std::shared_ptr<sadx::GameLogicController>{};continue;}
        auto pad=std::make_shared<sadx::GameLogicController>();
        for(std::uint32_t b=0;b<sadx::GameLogicController::byte_count;++b)pad->write_byte(b,pattern(seed,1+i,b));
        pad->write_u32(0x1c,in[7+i]);pad->write_u32(8,in[11+i]);pad->write_u32(0x10,in[15+i]);raw[i]=pad;
    }
    for(std::uint32_t slot=0;slot<8;++slot)for(std::uint32_t b=0;b<sadx::GameLogicController::byte_count;++b)pads.records[slot].write_byte(b,pattern(seed,8+slot,b));
    masks.mask_at_909f94=pattern32(seed,20,0);masks.mask_at_909fa4=pattern32(seed,20,1);
    for(std::uint32_t i=0;i<4;++i){
        keys.held_work_at_3b0e3a8[i]=in[19+i];keys.repeat_at_3b0e3bc[i]=static_cast<std::int32_t>(in[23+i]);keys.pressed_work_at_3b0e354[i]=in[27+i];
        keys.held_out_at_3b0e7e0[i]=pattern32(seed,21,i);keys.pressed_out_at_3b0e3cc[i]=pattern32(seed,22,i);
    }
    std::optional<std::uint16_t> stage=static_cast<std::uint16_t>(in[0]);std::optional<std::uint8_t> control=static_cast<std::uint8_t>(in[1]);
    std::array<std::optional<std::uint8_t>,4> enabled{{static_cast<std::uint8_t>(in[2]),static_cast<std::uint8_t>(in[3]),0,0}};
    sadx::SceneInputProducerState state{raw,pads,masks,keys,stage,control,[&](std::size_t i)->std::optional<std::uint8_t>{return enabled[i];}};std::uint32_t analog_calls=0;
    sadx::SceneInputProducerRuntime runtime{state,[&](sadx::X87Context&){++analog_calls;
        return in[5]?sadx::PlayerInputProducerResult{sadx::PlayerInputProducerFailure::unavailable_camera_yaw,0x40f170,0}:sadx::PlayerInputProducerResult{};}};
    sadx::X87Context fp(0x7f);const auto r=sadx::produce_scene_input(runtime,fp);
    std::fill(out,out+capacity,0);
    out[0]=static_cast<std::uint32_t>(r.failure);out[1]=r.source_boundary;out[2]=r.nested_source_boundary;out[3]=analog_calls;
    out[4]=masks.mask_at_909f94.value_or(0xdeadbeef);out[5]=masks.mask_at_909fa4.value_or(0xdeadbeef);
    for(std::uint32_t i=0;i<4;++i){
        out[6+i]=keys.held_work_at_3b0e3a8[i].value_or(0xdeadbeef);out[10+i]=keys.pressed_work_at_3b0e354[i].value_or(0xdeadbeef);
        out[14+i]=static_cast<std::uint32_t>(keys.repeat_at_3b0e3bc[i].value_or(0));out[18+i]=keys.held_out_at_3b0e7e0[i].value_or(0xdeadbeef);out[22+i]=keys.pressed_out_at_3b0e3cc[i].value_or(0xdeadbeef);
    }
    for(std::uint32_t slot=0;slot<8;++slot)for(std::uint32_t w=0;w<13;++w)out[26+slot*13+w]=pads.records[slot].read_u32(w*4).value_or(0xdeadbeef);
    return 1;
}
