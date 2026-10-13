// Source-local movement comparison with real input and matrix/floor callees.
// Explicit runtime seeds do not represent observed game initialization.
#include "player/player_movement.hpp"
#include <cstddef>
#include <cstring>
#include <utility>

namespace {
float value(std::uint32_t word) { float number;std::memcpy(&number,&word,4);return number; }
std::uint32_t bits(float number) { std::uint32_t word;std::memcpy(&word,&number,4);return word; }
std::int32_t signed_word(std::uint32_t word) { std::int32_t number;std::memcpy(&number,&word,4);return number; }
sadx::NinjaVector vector(const std::uint32_t* words) { return {value(words[0]),value(words[1]),value(words[2])}; }
}

extern "C" int sadx_native_player_movement(
    const std::uint8_t* table_bytes,std::size_t table_size,
    const std::uint8_t* unit_bytes,std::size_t unit_size,
    const std::uint8_t* constant_bytes,std::size_t constant_size,
    const std::uint32_t* words,std::size_t word_count,std::uint16_t control_word,
    std::uint32_t* output,std::size_t output_count) {
    if (!table_bytes || table_size!=4100 || !unit_bytes || unit_size!=64 ||
        !constant_bytes || constant_size!=36 || !words || word_count!=166 ||
        !output || output_count!=112 || words[58]>=8) return 1;
    try {
        sadx::NinjaMath::SineTable table;std::memcpy(table.data(),table_bytes,table_size);
        sadx::NinjaMatrix unit;std::memcpy(unit.data(),unit_bytes,unit_size);
        std::array<float,3> cuts;std::array<double,3> scales;
        std::memcpy(cuts.data(),constant_bytes,12);std::memcpy(scales.data(),constant_bytes+12,24);
        const sadx::FloorAngleConstants constants{cuts[0],cuts[1],cuts[2],scales[0],scales[1],scales[2]};
        std::vector<sadx::NinjaMatrix> frames(6);
        for(std::size_t r=0;r<6;++r) for(std::size_t c=0;c<16;++c) frames[r][c]=value(words[70+r*16+c]);
        sadx::NinjaMatrixStack stack(std::move(frames),words[68],words[69]);
        sadx::PlayerCoordinates coordinates(table,unit,stack);
        sadx::X87Context fp(control_word);
        const sadx::PlayerTaskState task{{static_cast<std::uint16_t>(words[0]),
            {signed_word(words[1]),signed_word(words[2]),signed_word(words[3])}},
            static_cast<std::uint8_t>(words[58]),static_cast<std::uint8_t>(words[59]),
            sadx::PlayerEventMoveData{static_cast<std::uint8_t>(words[60]),{},{},{}}};
        sadx::MotionWork world{vector(words+4),vector(words+7)};
        sadx::PlayerPhysicsState player{{vector(words+13),vector(words+10)},value(words[16]),
            vector(words+17),vector(words+20),words[23],words[24]};
        const auto decoded=sadx::decode_player_parameters(reinterpret_cast<const std::uint8_t*>(words+25),132);
        if(!decoded) return 2;
        sadx::PlayerInputRuntime input;
        input.stage=static_cast<std::int16_t>(words[61]);input.act=static_cast<std::int16_t>(words[62]);
        input.current_character=static_cast<std::uint16_t>(words[63]);
        input.normalized[task.player_index]=sadx::NormalizedPlayerInput{signed_word(words[64]),value(words[65])};
        input.no_control_timer[task.player_index]=static_cast<std::uint16_t>(words[66]);
        input.camera_yaw=signed_word(words[67]);
        const sadx::CrtAsinRuntime math{1,{}}; // Explicit dispatcher-bypass fixture.
        const auto result=sadx::run_player_movement(task,input,world,player,*decoded.parameters,
            coordinates,constants,math,fp);
        output[0]=bits(player.motion.speed.x);output[1]=bits(player.motion.speed.y);output[2]=bits(player.motion.speed.z);
        output[3]=bits(world.speed.x);output[4]=bits(world.speed.y);output[5]=bits(world.speed.z);
        output[6]=result.input_query_count;output[7]=fp.exception_flags();
        output[8]=static_cast<std::uint32_t>(result.speed.failure);output[9]=stack.depth();
        output[10]=bits(world.acceleration.x);output[11]=bits(world.acceleration.y);output[12]=bits(world.acceleration.z);
        output[13]=bits(player.motion.acceleration.x);output[14]=bits(player.motion.acceleration.y);output[15]=bits(player.motion.acceleration.z);
        for(std::size_t r=0;r<6;++r) for(std::size_t c=0;c<16;++c) output[16+r*16+c]=bits(stack.slots()[r][c]);
        return 0;
    } catch(const sadx::X87Error&) { return 3; }
      catch(const sadx::CoordinateError&) { return 4; }
      catch(...) { return 5; }
}
