#pragma once
#include <cstdint>

namespace sadx::gamma_input_buffer {
// NOTSA: ordered access to E102WK.input_buf[7] and the existing input_dataG
// owner. Separate operations preserve a completed angle store if stroke fails.
struct Access {
    virtual ~Access() = default;
    virtual std::int32_t buffered_angle() = 0;
    virtual void store_angle(std::int32_t player, std::int32_t angle) = 0;
    virtual std::uint32_t buffered_stroke_bits() = 0;
    virtual void store_stroke_bits(std::int32_t player, std::uint32_t bits) = 0;
};
// The earlier descriptive symbol name predates recovery: this restores the
// latest buffered analog input, without inspecting laser targets or task lists.
// FUNCTION: SONIC 0x4c3870 GammaUpdateLaserTargets
void restore(Access &, std::int32_t player);
} // namespace sadx::gamma_input_buffer
