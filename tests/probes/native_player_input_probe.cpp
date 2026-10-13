// Function-local probe. Runtime globals are explicit external test seeds;
// this does not observe a live game or execute the updated mods' DLLs.
#include "player/player_input.hpp"
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace {
float value(std::uint32_t word) { float result; std::memcpy(&result,&word,4); return result; }
std::int32_t signed_value(std::uint32_t word) { std::int32_t result; std::memcpy(&result,&word,4); return result; }
std::uint32_t bits(float value) { std::uint32_t result; std::memcpy(&result,&value,4); return result; }
}

extern "C" int sadx_native_player_input(const std::uint32_t* words, std::size_t count,
                                         std::uint16_t control_word, std::uint32_t output_mask,
                                         const double* angle_constants, std::size_t constant_count,
                                         std::uint32_t* output, std::size_t output_count) {
    if (!words || count!=12 || !output || output_count!=5 || output_mask>3 ||
        !angle_constants || constant_count!=4) return 1;
    try {
        sadx::X87Context fp(control_word);
        const sadx::PlayerInputTask task{static_cast<std::uint16_t>(words[0]),
            static_cast<std::uint8_t>(words[1]),static_cast<std::uint8_t>(words[2]),signed_value(words[3]),
            static_cast<std::uint8_t>(words[4])};
        sadx::PlayerInputRuntime runtime;
        runtime.stage=static_cast<std::int16_t>(words[5]); runtime.act=static_cast<std::int16_t>(words[6]);
        runtime.current_character=static_cast<std::uint16_t>(words[7]); runtime.camera_yaw=signed_value(words[11]);
        if (task.player_index<8) {
            runtime.normalized[task.player_index]=sadx::NormalizedPlayerInput{signed_value(words[8]),value(words[9])};
            runtime.no_control_timer[task.player_index]=static_cast<std::uint16_t>(words[10]);
        }
        std::int32_t angle=signed_value(0xa1b2c3d4); float magnitude=value(0xdeadbeef);
        const sadx::ForcedInputAngleConstants constants{angle_constants[0],angle_constants[1],
            angle_constants[2],angle_constants[3]};
        const sadx::ForcedInputAngleConversion conversion=[&](sadx::X87Context& context) {
            return sadx::convert_forced_input_angle(constants,context);
        };
        const auto result=sadx::get_player_input(task,runtime,fp,(output_mask&1)?&angle:nullptr,
                                                (output_mask&2)?&magnitude:nullptr,conversion);
        output[0]=result.active.has_value()?static_cast<std::uint32_t>(*result.active):0xffffffff;
        output[1]=static_cast<std::uint32_t>(angle); output[2]=bits(magnitude);
        output[3]=fp.exception_flags(); output[4]=static_cast<std::uint32_t>(result.failure);
        return 0;
    } catch (const sadx::X87Error&) { return 2; }
      catch (...) { return 3; }
}
