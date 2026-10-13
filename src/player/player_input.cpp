#include "player/player_input.hpp"
#include "cpu/crt_integer.hpp"
#include "cpu/x87_transcendental.hpp"

#include <cstring>

namespace sadx {
namespace {
PlayerInputResult unavailable(PlayerInputFailure failure, std::uint32_t address,
                              std::optional<bool> active = {}) {
    return {failure, address, active};
}
void store_angle(std::int32_t* output, std::int32_t value) {
    if (output) std::memcpy(output, &value, sizeof(value));
}
void store_magnitude(float* output, const float& value) {
    if (output) std::memcpy(output, &value, sizeof(value));
}
std::int32_t signed_word(std::uint32_t bits) {
    std::int32_t value;
    std::memcpy(&value, &bits, sizeof(value));
    return value;
}
} // namespace

ForcedInputAngleResult convert_forced_input_angle(const ForcedInputAngleConstants& constants,
                                                 X87Context& fp) {
    const auto y=fp.from_double(constants.y);
    const auto x=fp.from_double(constants.x);
    const auto angle=X87Transcendental::fpatan_bochs_p5_p6(fp,y,x);
    const auto units=fp.multiply(angle,fp.from_double(constants.angle_units));
    const auto scaled=fp.multiply(units,fp.from_double(constants.factor));
    return {PlayerInputFailure::none,signed_word(crt_ftol_low32(scaled,fp))};
}

PlayerInputResult get_player_input(const PlayerInputTask& task,
                                  const PlayerInputRuntime& runtime,
                                  X87Context& fp, std::int32_t* angle,
                                  float* magnitude,
                                  const ForcedInputAngleConversion& forced_angle) {
    if ((task.flags & 0x4000U) != 0) {
        if (!task.event_move_mode)
            return unavailable(PlayerInputFailure::unavailable_event_move_mode, 0x43bdd8);
        return {PlayerInputFailure::none, 0, *task.event_move_mode != 0};
    }
    if (!runtime.stage)
        return unavailable(PlayerInputFailure::unavailable_stage,
                           (task.flags & 0x2000U) != 0 ? 0x43bdf1 : 0x43be6d);

    if ((task.flags & 0x2000U) != 0 && (*runtime.stage == 2 || *runtime.stage == 4)) {
        if (!runtime.act)
            return unavailable(PlayerInputFailure::unavailable_act,
                               *runtime.stage == 2 ? 0x43bdfe : 0x43be0f);
        if ((*runtime.stage == 2 && *runtime.act == 2) ||
            (*runtime.stage == 4 && *runtime.act == 0)) {
            if (!runtime.current_character)
                return unavailable(PlayerInputFailure::unavailable_current_character, 0x43be19);
            if (*runtime.current_character == 0) {
                if (angle) {
                    if (!forced_angle)
                        return unavailable(PlayerInputFailure::missing_forced_angle_conversion, 0x43be36, true);
                    const auto converted = forced_angle(fp);
                    if (converted.failure != PlayerInputFailure::none || !converted.angle_units)
                        return unavailable(converted.failure == PlayerInputFailure::none
                                               ? PlayerInputFailure::unavailable_forced_angle_conversion
                                               : converted.failure,
                                           0x43be36, true);
                    if (!runtime.camera_yaw)
                        return unavailable(PlayerInputFailure::unavailable_camera_yaw, 0x43be49, true);
                    // NEG/SUB on EDX use modulo-2^32, including INT_MIN.
                    const auto value = 0U - static_cast<std::uint32_t>(*runtime.camera_yaw)
                                       - static_cast<std::uint32_t>(*converted.angle_units);
                    store_angle(angle, signed_word(value));
                }
                store_magnitude(magnitude, 1.0F);
                return {PlayerInputFailure::none, 0, true,
                    static_cast<std::uint8_t>((angle ? 1U : 0U)|(magnitude ? 2U : 0U))};
            }
        }
    }

    if (task.player_index >= runtime.normalized.size())
        return unavailable(PlayerInputFailure::player_index_outside_known_slots, 0x43be78);
    const auto& input = runtime.normalized[task.player_index];
    if (!input)
        return unavailable(PlayerInputFailure::unavailable_normalized_input, 0x43be78);
    // FCOMP + TEST AH,0x44 + JNP rejects only ordered equality to zero.
    // Negative and unordered magnitudes follow the active branch. FCOM (not
    // FUCOM) raises invalid for every NaN, as modeled by the selected context.
    const auto source_magnitude = fp.from_float(input->magnitude);
    const auto comparison = fp.compare(source_magnitude, fp.from_float(0.0F));
    if (comparison != X87Comparison::equal) {
        if ((task.flags & 0x2000U) == 0) {
            const auto timer = runtime.no_control_timer[task.player_index];
            if (!timer)
                return unavailable(PlayerInputFailure::unavailable_no_control_timer, 0x43be9c);
            if (*timer != 0) return {PlayerInputFailure::none, 0, false};
        }
        if (angle) {
            if (!input->angle)
                return unavailable(PlayerInputFailure::unavailable_normalized_angle, 0x43beb5, true);
            store_angle(angle, *input->angle);
        }
        store_magnitude(magnitude, input->magnitude);
        return {PlayerInputFailure::none, 0, true,
            static_cast<std::uint8_t>((angle ? 1U : 0U)|(magnitude ? 2U : 0U))};
    }

    if (task.character_id == 0 && (task.flags & 0x100U) != 0) {
        if (!runtime.act)
            return unavailable(PlayerInputFailure::unavailable_act, 0x43bef1);
        // Original MOVSX/SHL/OR uses both signed 16-bit globals, not an
        // unsigned stage/act concatenation. Negative act sign-extends.
        const auto packed = (static_cast<std::uint32_t>(static_cast<std::int32_t>(*runtime.stage)) << 8U)
                            | static_cast<std::uint32_t>(static_cast<std::int32_t>(*runtime.act));
        if (packed == 0x902U || packed == 0x903U) {
            store_angle(angle, task.yaw);
            store_magnitude(magnitude, 0.0F);
            return {PlayerInputFailure::none, 0, true,
                static_cast<std::uint8_t>((angle ? 1U : 0U)|(magnitude ? 2U : 0U))};
        }
    }
    return {PlayerInputFailure::none, 0, false};
}

} // namespace sadx
