#pragma once

#include "cpu/fidelity_float.hpp"

#include <cstdint>

namespace sadx::gamma_gun_effects {
// NOTSA: the E102WK fields GammaUpdateGunEffects reads and writes.
// The lock walk and the volley stay behind Services.
struct GunState {
    std::int8_t laser_mode = 0;
    std::int8_t mode = 0;
    std::int8_t marker = 0;
    std::int8_t lock_count = 0;
    std::int8_t armed = 0;
    std::int8_t variant = 0;
    float charge = 0;
    std::uint32_t variant_value = 0;
    float fade = 0;
};

// Callees that are not part of this body. An empty lock list never calls them:
// the source helper returns null and clears the marker.
struct Services {
    virtual ~Services() = default;
    // STUB owner: SONIC 0x4c38d0 once the lock count is positive.
    virtual std::uintptr_t select_lock(int pass) = 0;
    // STUB owner: SONIC 0x4c3e40.
    virtual std::uint8_t read_variant(std::uintptr_t selected) = 0;
    // GLOBAL: SONIC 0x3c5b2e4. The file leaves this table zero.
    virtual std::uint32_t variant_word(std::uint8_t variant) = 0;
    // STUB owner: the volley that starts at the CreateElementalTask in mode 2 / 5.
    virtual void open_volley(GunState &state, std::uintptr_t selected) = 0;
};

// FUNCTION: SONIC 0x4c44a0 GammaUpdateGunEffects
void update(GunState &state, Services &services, fidelity::Context &arithmetic);
} // namespace sadx::gamma_gun_effects
