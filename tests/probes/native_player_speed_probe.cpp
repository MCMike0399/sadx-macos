// Function-local test boundary. The input gate and both transforms are
// explicit test doubles; this does not verify the game's real callees.
#include "player/player_speed.hpp"
#include <cstddef>
#include <cstdint>
#include <cstring>

static float value(std::uint32_t bits) {
    float result;
    std::memcpy(&result,&bits,4);
    return result;
}
static std::uint32_t bits(float value) {
    std::uint32_t result;
    std::memcpy(&result,&value,4);
    return result;
}
static sadx::NinjaVector vector(const std::uint32_t* words) {
    return {value(words[0]),value(words[1]),value(words[2])};
}

extern "C" int sadx_native_player_speed(const std::uint32_t* words, std::size_t count,
                                         std::uint16_t control_word, std::uint32_t gate_pattern,
                                         std::uint32_t* output, std::size_t output_count) {
    if(!words || count!=58 || !output || output_count!=16) return 1;
    try {
    sadx::X87Context fp(control_word);
    const sadx::PlayerOrientation orientation{static_cast<std::uint16_t>(words[0]),
        {static_cast<std::int32_t>(words[1]),static_cast<std::int32_t>(words[2]),static_cast<std::int32_t>(words[3])}};
    sadx::MotionWork world{vector(words+4),vector(words+7)};
    sadx::PlayerPhysicsState player{{vector(words+13),vector(words+10)},value(words[16]),
        vector(words+17),vector(words+20),words[23],words[24]};
    const auto decoded=sadx::decode_player_parameters(reinterpret_cast<const std::uint8_t*>(words+25),132);
    if(!decoded) return 2;
    unsigned gate_calls=0;
    sadx::PlayerSpeedCallees callees;
    callees.input_gate=[&](const sadx::PlayerOrientation&,sadx::X87Context&) {
        if (gate_calls >= 32) return sadx::PlayerInputGateResult{
            sadx::PlayerSpeedFailure::input_gate_unavailable,{}};
        const bool active=(gate_pattern & (1U<<gate_calls)) != 0;
        ++gate_calls;
        return sadx::PlayerInputGateResult{sadx::PlayerSpeedFailure::none,active};
    };
    callees.task_to_world=[](const sadx::PlayerOrientation&,sadx::NinjaVector&,sadx::X87Context&) {
        return sadx::PlayerSpeedFailure::none;
    };
    callees.floor_to_world=[](const sadx::NinjaVector&,sadx::NinjaVector&,std::int32_t,sadx::X87Context&) {
        return sadx::PlayerSpeedFailure::none;
    };
    const auto result=sadx::get_player_speed(orientation,world,player,*decoded.parameters,callees,fp);
    output[0]=bits(player.motion.speed.x); output[1]=bits(player.motion.speed.y); output[2]=bits(player.motion.speed.z);
    output[3]=bits(world.speed.x); output[4]=bits(world.speed.y); output[5]=bits(world.speed.z);
    output[6]=gate_calls; output[7]=fp.exception_flags();
    output[8]=static_cast<std::uint32_t>(result.failure); output[9]=result.source_boundary;
    output[10]=bits(world.acceleration.x); output[11]=bits(world.acceleration.y); output[12]=bits(world.acceleration.z);
    output[13]=bits(player.motion.acceleration.x); output[14]=bits(player.motion.acceleration.y); output[15]=bits(player.motion.acceleration.z);
    return 0;
    } catch (const sadx::X87Error&) { return 3; }
      catch (...) { return 4; }
}
