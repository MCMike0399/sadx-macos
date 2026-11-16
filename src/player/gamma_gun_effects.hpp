#pragma once

#include "cpu/fidelity_float.hpp"

#include <cstdint>

namespace sadx::gamma_gun_effects {
// Source E102WK +1 mode byte, signed. The original sign-extends the byte and
// compares it against 5 with an unsigned branch, so a value outside 0..5
// (including a negative byte) takes the switch default and finishes.
// The recovered body establishes only the numeric arms; no source role name
// survives for any of them.
enum class Mode : std::int8_t {
    Mode0 = 0, // clears the fade and falls through to Mode1
    Mode1 = 1, // selects a lock; a valid selection starts the first volley
    Mode2 = 2, // reopens the volley with a null selection
    Mode3 = 3, // selects a lock, then enters Mode4
    Mode4 = 4, // advances the charge; 15 enters Mode5
    Mode5 = 5, // cools down; 30 returns to Mode3
};

// NOTSA: the E102WK fields GammaUpdateGunEffects reads and writes.
// The lock walk and the volley stay behind Services.
struct GunState {
    std::int8_t laser_mode = 0;
    Mode mode = Mode::Mode0;
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
    // The lock selector is unrecovered once the lock count is positive. The
    // source passes 0 or 1; what the pass selects stays inside that helper.
    virtual std::uintptr_t select_lock(int pass) = 0;
    // The variant reader is unrecovered.
    virtual std::uint8_t read_variant(std::uintptr_t selected) = 0;
    // The image leaves the variant table zero.
    virtual std::uint32_t variant_word(std::uint8_t variant) = 0;
    // STUB owner: the volley that starts at the CreateElementalTask in mode 2 / 5.
    virtual void open_volley(GunState &state, std::uintptr_t selected) = 0;
};

// FUNCTION: SONIC 0x4c44a0 GammaUpdateGunEffects
void update(GunState &state, Services &services, fidelity::Context &arithmetic);
} // namespace sadx::gamma_gun_effects
