// Original-instruction comparison boundary, not a gameplay adapter.
#include "player/player_coordinates.hpp"
#include <cstddef>
#include <cstring>

namespace {
float number(std::uint32_t word) { float value;std::memcpy(&value,&word,4);return value; }
std::uint32_t bits(float value) { std::uint32_t word;std::memcpy(&word,&value,4);return word; }
std::int32_t angle(std::uint32_t word) { std::int32_t value;std::memcpy(&value,&word,4);return value; }
}

extern "C" int sadx_native_coordinates(const std::uint8_t* table_bytes,std::size_t table_size,
                                         const std::uint8_t* unit_bytes,std::size_t unit_size,
                                         const std::uint32_t* words,std::size_t word_count,
                                         std::uint16_t control_word,
                                         std::uint32_t* output,std::size_t output_count) {
    if (!table_bytes || table_size!=4100 || !unit_bytes || unit_size!=64 ||
        !words || word_count!=106 || !output || output_count!=102) return 1;
    try {
        sadx::NinjaMath::SineTable table;std::memcpy(table.data(),table_bytes,table_size);
        sadx::NinjaMatrix unit;std::memcpy(unit.data(),unit_bytes,unit_size);
        std::vector<sadx::NinjaMatrix> frames(6);
        for (std::size_t r=0;r<frames.size();++r) for (std::size_t c=0;c<16;++c) frames[r][c]=number(words[10+r*16+c]);
        sadx::NinjaMatrixStack stack(std::move(frames),words[7],words[8]);
        sadx::X87Context fp(control_word);sadx::PlayerCoordinates coords(table,unit,stack);
        const sadx::PlayerOrientation task{0,{angle(words[1]),angle(words[2]),angle(words[3])}};
        sadx::NinjaVector vector{number(words[4]),number(words[5]),number(words[6])};
        sadx::CoordinateResult result;
        switch (words[0]) {
        case 0: result=coords.world_to_player(task,vector,fp);break;
        case 1: result=coords.task_to_world(words[9] ? nullptr : &task,vector,fp);break;
        case 2: coords.rotate_x(stack.current(),task.angles[0],fp);break;
        case 3: coords.rotate_y(stack.current(),task.angles[1],fp);break;
        case 4: coords.rotate_z(stack.current(),task.angles[2],fp);break;
        case 5: result=coords.calculate_vector(stack.current(),vector,vector,fp);break;
        default:return 2;
        }
        output[0]=bits(vector.x);output[1]=bits(vector.y);output[2]=bits(vector.z);
        output[3]=stack.depth();output[4]=fp.exception_flags();output[5]=static_cast<std::uint32_t>(result.failure);
        for (std::size_t r=0;r<stack.slots().size();++r) for (std::size_t c=0;c<16;++c) output[6+r*16+c]=bits(stack.slots()[r][c]);
        return 0;
    } catch (const sadx::X87Error&) { return 3; }
      catch (const sadx::CoordinateError&) { return 4; }
      catch (...) { return 5; }
}
