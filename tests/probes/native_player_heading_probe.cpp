#include "player/player_heading.hpp"
#include <cstddef>
#include <cstring>
#include <utility>

namespace {
std::int32_t signed_word(std::uint32_t word) { std::int32_t value;std::memcpy(&value,&word,4);return value; }
float number(std::uint32_t word) { float value;std::memcpy(&value,&word,4);return value; }
std::uint32_t bits(float value) { std::uint32_t word;std::memcpy(&word,&value,4);return word; }
}
extern "C" std::uint32_t sadx_native_adjust_angle(std::uint32_t from,std::uint32_t to,std::uint32_t step) {
    return static_cast<std::uint32_t>(sadx::adjust_angle(signed_word(from),signed_word(to),signed_word(step)));
}
extern "C" int sadx_native_player_heading(
    const std::uint8_t* table_bytes,std::size_t table_size,
    const std::uint8_t* unit_bytes,std::size_t unit_size,
    const std::uint32_t* words,std::size_t word_count,std::uint16_t control_word,
    std::uint32_t* output,std::size_t output_count) {
    if(!table_bytes || table_size!=4100 || !unit_bytes || unit_size!=64 ||
        !words || word_count!=106 || !output || output_count!=103) return 1;
    try {
        sadx::NinjaMath::SineTable table;std::memcpy(table.data(),table_bytes,table_size);
        sadx::NinjaMatrix unit;std::memcpy(unit.data(),unit_bytes,unit_size);
        std::vector<sadx::NinjaMatrix> frames(6);
        for(std::size_t r=0;r<6;++r) for(std::size_t c=0;c<16;++c) frames[r][c]=number(words[10+r*16+c]);
        sadx::NinjaMatrixStack stack(std::move(frames),words[7],words[8]);
        sadx::PlayerCoordinates coordinates(table,unit,stack);sadx::X87Context fp(control_word);
        sadx::PlayerOrientation task{0,{signed_word(words[0]),signed_word(words[1]),signed_word(words[2])}};
        sadx::PlayerMotion motion{{0,0,0},{number(words[4]),number(words[5]),number(words[6])}};
        const auto result=sadx::turn_player_heading(task,motion,signed_word(words[3]),coordinates,fp);
        output[0]=static_cast<std::uint32_t>(task.angles[1]);
        output[1]=bits(motion.speed.x);output[2]=bits(motion.speed.y);output[3]=bits(motion.speed.z);
        output[4]=stack.depth();output[5]=fp.exception_flags();output[6]=static_cast<std::uint32_t>(result.coordinate.failure);
        for(std::size_t r=0;r<6;++r) for(std::size_t c=0;c<16;++c) output[7+r*16+c]=bits(stack.slots()[r][c]);
        return 0;
    } catch(const sadx::X87Error&) { return 2; }
      catch(const sadx::CoordinateError&) { return 3; }
      catch(...) { return 4; }
}
