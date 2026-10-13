#include "player/player_input_producer.hpp"
#include "cpu/crt_integer.hpp"
#include "math/ninja_math.hpp"
#include "cpu/x87_transcendental.hpp"

#include <cstring>
#include <type_traits>

namespace sadx {
namespace {
PlayerInputProducerResult unavailable(PlayerInputProducerFailure failure,
                                      std::uint32_t address, unsigned slot) {
    return {failure,address,static_cast<std::uint8_t>(slot)};
}
template<class Value>
Value little_endian_value(const std::uint8_t* bytes) {
    using Word = std::conditional_t<sizeof(Value)==4,std::uint32_t,std::uint64_t>;
    Word bits=0;
    for(unsigned i=0;i<sizeof(Value);++i) bits|=static_cast<Word>(bytes[i])<<(8U*i);
    Value value;std::memcpy(&value,&bits,sizeof(value));return value;
}
bool beyond_deadzone(float value,const PlayerInputProducerConstants& c,X87Context& fp) {
    if(fp.compare_float_memory(fp.from_float(value),c.positive_deadzone)==X87Comparison::greater) return true;
    return fp.compare_float_memory(fp.from_float(value),c.negative_deadzone)==X87Comparison::less;
}
X87Value adjusted_axis(float value,const PlayerInputProducerConstants& c,X87Context& fp) {
    const auto positive=fp.compare_float_memory(fp.from_float(value),c.positive_deadzone);
    const auto loaded=fp.from_float(value); // source reload after comparison
    if(positive==X87Comparison::greater) return fp.subtract_float_memory(loaded,c.positive_deadzone);
    const auto negative=fp.compare_float_memory(loaded,c.negative_deadzone);
    if(negative==X87Comparison::less) return fp.add_float_memory(fp.from_float(value),c.positive_deadzone);
    return fp.from_float(0);
}
int absolute_axis(std::int16_t value) {
    const int wide=value;return wide<0 ? -wide : wide;
}
} // namespace

std::optional<PlayerInputProducerConstants> decode_player_input_producer_constants(
    const std::uint8_t* bytes,std::size_t size) {
    if(!bytes || size!=40) return {};
    return PlayerInputProducerConstants{
        little_endian_value<float>(bytes),little_endian_value<float>(bytes+4),
        little_endian_value<float>(bytes+8),little_endian_value<float>(bytes+12),
        little_endian_value<double>(bytes+16),little_endian_value<double>(bytes+24),
        little_endian_value<double>(bytes+32)};
}

PlayerInputProducerResult write_analogs_bochs_p5_p6(
    const PlayerInputProducerRuntime& runtime,const PlayerInputProducerConstants& c,
    PlayerInputProducedState& state,X87Context& fp) {
    for(unsigned slot=0;slot<8;++slot) {
        std::uint32_t direction=0;
        std::optional<X87Value> magnitude;
        if(slot<2) {
            if(!runtime.raw_controllers || !(*runtime.raw_controllers)[slot])
                return unavailable(PlayerInputProducerFailure::unavailable_raw_controller,0x40f185,slot);
            const auto& pad=*(*runtime.raw_controllers)[slot];
            if(!pad) return unavailable(PlayerInputProducerFailure::null_raw_controller,0x40f18c,slot);
            const auto raw_x=pad->read_u16(GameLogicController::stick_x_offset);
            if(!raw_x) return unavailable(PlayerInputProducerFailure::unavailable_raw_axis,0x40f18c,slot);
            const auto raw_y=pad->read_u16(GameLogicController::stick_y_offset);
            if(!raw_y) return unavailable(PlayerInputProducerFailure::unavailable_raw_axis,0x40f190,slot);
            const PlayerInputPadAxes axes{static_cast<std::int16_t>(*raw_x),static_cast<std::int16_t>(*raw_y)};
            const float x=fp.to_float(fp.from_int64(static_cast<std::int32_t>(axes.x)*256));
            const float y=fp.to_float(fp.from_int64(static_cast<std::int32_t>(axes.y)*256));
            if(beyond_deadzone(x,c,fp) || beyond_deadzone(y,c,fp)) {
                const auto adjusted_x=adjusted_axis(x,c,fp);
                const auto adjusted_y=adjusted_axis(y,c,fp);
                if(!runtime.camera)
                    return unavailable(PlayerInputProducerFailure::unavailable_camera_presence,0x40f27d,slot);
                const auto atan=X87Transcendental::fpatan_bochs_p5_p6(fp,adjusted_y,adjusted_x);
                const auto units=fp.multiply(atan,fp.from_double(c.angle_units));
                const auto scaled=fp.multiply(units,fp.from_double(runtime.camera->present
                                                                  ? c.negative_factor : c.positive_factor));
                const auto converted=crt_ftol_low32(scaled,fp);
                if(runtime.camera->present) {
                    if(!runtime.camera->yaw)
                        return unavailable(PlayerInputProducerFailure::unavailable_camera_yaw,0x40f29c,slot);
                    direction=0U-static_cast<std::uint32_t>(*runtime.camera->yaw)-converted;
                } else direction=converted;
                // Source: duplicate Y/Y, multiply; duplicate X/X, multiply;
                // FADDP adds X squared to Y squared, then binary32 store.
                const auto squared_y=fp.multiply(adjusted_y,adjusted_y);
                const auto squared_x=fp.multiply(adjusted_x,adjusted_x);
                const float squared=fp.to_float(fp.add(squared_y,squared_x));
                // SignedSqrt0x7871f0 RET4 consumes the pushed squared word,
                // so caller FMUL [ESP+0x14] uses that same stored squared sum.
                const auto root=signed_sqrt_extended(squared,fp);
                magnitude=fp.multiply_float_memory(fp.multiply_float_memory(root,squared),c.magnitude_scale);
                if(fp.compare_float_memory(*magnitude,c.magnitude_cap)==X87Comparison::greater)
                    magnitude=fp.from_float(c.magnitude_cap);
            } else magnitude=fp.from_float(0);
        } else magnitude=fp.from_float(0);
        if(!runtime.control_enabled)
            return unavailable(PlayerInputProducerFailure::unavailable_control_enabled,0x40f2ed,slot);
        // FST raw magnitude retains its extended register for later FSTP.
        const float raw_magnitude=fp.to_float(*magnitude);
        const auto masked=static_cast<std::int32_t>(direction&0xffffU);
        state.raw_unused[slot]=NormalizedPlayerInput{masked,raw_magnitude};
        if(*runtime.control_enabled==1) {
            if(!runtime.controller_enabled[slot])
                return unavailable(PlayerInputProducerFailure::unavailable_controller_enabled,0x40f30a,slot);
            if(*runtime.controller_enabled[slot]==1) {
                state.normalized[slot]=NormalizedPlayerInput{masked,fp.to_float(*magnitude)};
                continue;
            }
        }
        state.normalized[slot]=NormalizedPlayerInput{0,0.0F};
    }
    return {};
}

PlayerInputProducerResult override_input_mod_261_magnitudes(
    const PlayerInputProducerRuntime& runtime,PlayerInputProducedState& state) {
    if(!runtime.control_enabled)
        return unavailable(PlayerInputProducerFailure::unavailable_control_enabled,0x10004b00,0);
    if(*runtime.control_enabled==0) return {};
    for(unsigned slot=0;slot<8;++slot) {
        if(!runtime.controller_enabled[slot])
            return unavailable(PlayerInputProducerFailure::unavailable_controller_enabled,0x10004b10,slot);
        if(*runtime.controller_enabled[slot]==0) continue;
        const auto& pad=runtime.input_mod_pads[slot];
        if(!pad) return unavailable(PlayerInputProducerFailure::unavailable_input_mod_pad,0x10004b19,slot);
        if(!pad->connected) return unavailable(PlayerInputProducerFailure::unavailable_connection,0x10004b19,slot);
        if(*pad->connected==0) {
            if(!pad->keyboard_allowed)
                return unavailable(PlayerInputProducerFailure::unavailable_keyboard_allowed,0x10004b22,slot);
            if(*pad->keyboard_allowed==0 || slot!=0) continue;
        }
        if(!pad->axes) return unavailable(PlayerInputProducerFailure::unavailable_input_mod_axes,0x10004b2f,slot);
        if(absolute_axis(pad->axes->x)<=12 && absolute_axis(pad->axes->y)<=12) continue;
        if(!pad->normalized_magnitude)
            return unavailable(PlayerInputProducerFailure::unavailable_input_mod_magnitude,0x10004b51,slot);
        // The binary hook is an integer MOV of the original magnitude word,
        // including payload/signed-zero bits, not an arithmetic float store.
        float copied;
        std::memcpy(&copied,&*pad->normalized_magnitude,4);
        if(state.normalized[slot]) std::memcpy(&state.normalized[slot]->magnitude,&copied,4);
        else state.normalized[slot]=NormalizedPlayerInput{{},copied};
    }
    return {};
}

PlayerInputProducerResult produce_input_mod_261_bochs_p5_p6(
    const PlayerInputProducerRuntime& runtime,const PlayerInputProducerConstants& constants,
    PlayerInputProducedState& state,X87Context& fp) {
    const auto original=write_analogs_bochs_p5_p6(runtime,constants,state,fp);
    if(!original) return original;
    return override_input_mod_261_magnitudes(runtime,state);
}
} // namespace sadx
