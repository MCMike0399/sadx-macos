#include "player/player_input_producer.hpp"
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace {
float value(std::uint32_t word) { float v;std::memcpy(&v,&word,4);return v; }
std::int32_t signed_value(std::uint32_t word) { std::int32_t v;std::memcpy(&v,&word,4);return v; }
std::uint32_t bits(float v) { std::uint32_t w;std::memcpy(&w,&v,4);return w; }
}
extern "C" int sadx_native_player_input_producer(
    const std::uint32_t* words,std::size_t count,const std::uint8_t* constants,
    std::size_t constants_size,std::uint16_t control_word,bool updated,
    std::uint32_t* output,std::size_t output_count) {
    if(!words || count!=55 || !output || output_count!=36) return 1;
    const auto parsed=sadx::decode_player_input_producer_constants(constants,constants_size);
    if(!parsed) return 2;
    try {
        sadx::X87Context fp(control_word);sadx::PlayerInputProducerRuntime r;
        r.control_enabled=static_cast<std::uint8_t>(words[0]);
        r.camera=sadx::PlayerInputCamera{words[1]!=0,signed_value(words[2])};
        sadx::GameLogicControllers raw;r.raw_controllers=&raw;
        for(unsigned slot=0;slot<2;slot++) {
            auto pad=std::make_shared<sadx::GameLogicController>();
            pad->write_u16(0x1c,static_cast<std::uint16_t>(words[3+slot*2]));pad->write_u16(0x1e,static_cast<std::uint16_t>(words[4+slot*2]));
            raw[slot]=pad;
        }
        for(unsigned slot=0;slot<8;slot++) {
            r.controller_enabled[slot]=static_cast<std::uint8_t>(words[7+slot]);
            const auto* mod=words+15+slot*5;
            r.input_mod_pads[slot]=sadx::InputMod261Pad{static_cast<std::uint8_t>(mod[0]),
                static_cast<std::uint8_t>(mod[1]),sadx::PlayerInputPadAxes{
                    static_cast<std::int16_t>(mod[2]),static_cast<std::int16_t>(mod[3])},value(mod[4])};
        }
        sadx::PlayerInputProducedState state;
        const auto result=updated ? sadx::produce_input_mod_261_bochs_p5_p6(r,*parsed,state,fp)
                                  : sadx::write_analogs_bochs_p5_p6(r,*parsed,state,fp);
        for(unsigned slot=0;slot<8;slot++) {
            if(!state.raw_unused[slot] || !state.normalized[slot] ||
               !state.raw_unused[slot]->angle || !state.normalized[slot]->angle) return 3;
            output[slot*2]=static_cast<std::uint32_t>(*state.raw_unused[slot]->angle);
            output[slot*2+1]=bits(state.raw_unused[slot]->magnitude);
            output[16+slot*2]=static_cast<std::uint32_t>(*state.normalized[slot]->angle);
            output[17+slot*2]=bits(state.normalized[slot]->magnitude);
        }
        output[32]=fp.exception_flags();output[33]=static_cast<std::uint32_t>(result.failure);
        output[34]=result.source_boundary;output[35]=result.slot;return 0;
    } catch(const sadx::X87Error&) { return 4; }
      catch(...) { return 5; }
}
