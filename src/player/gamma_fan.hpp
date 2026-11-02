#pragma once

#include "cpu/fidelity_float.hpp"

namespace sadx::gamma_fan {
// NOTSA: ordered borrows of playerwk.spd and the existing E102WK allocation.
// The forward speed is deliberately reloaded after the sine call.
struct Access {
    virtual ~Access() = default;
    virtual float lateral_speed() = 0;
    virtual float forward_speed() = 0;
    virtual std::uint32_t phase() = 0;
    virtual void store_phase(std::uint32_t) = 0;
    virtual fidelity::ExactFloat sine(std::int32_t) = 0;
    virtual void store_height(float) = 0;
    virtual void store_tilt(std::int32_t) = 0;
};

// FUNCTION: SONIC 0x4c2e90 GammaUpdateFan
void update(Access &, fidelity::Context &);
} // namespace sadx::gamma_fan
