#include "player/gamma_gun_effects.hpp"

#include <cstdint>
#include <cstring>

namespace {
enum Input : unsigned {
    Mode = 0, Laser, Marker, LockCount, Armed, Variant, Charge, Value, Fade, ControlWord, Words
};
enum Output : unsigned {
    OutMode = 0, OutLaser, OutMarker, OutLock, OutArmed, OutVariant,     OutCharge, OutValue, OutFade, OutWords
};

struct Adapter final : sadx::gamma_gun_effects::Services {
    std::uintptr_t select_lock(int) override { return 0; }
    std::uint8_t read_variant(std::uintptr_t) override { return 0; }
    std::uint32_t variant_word(std::uint8_t) override { return 0; }
    void open_volley(sadx::gamma_gun_effects::GunState &, std::uintptr_t) override {}
};

std::int8_t byte_at(std::uint32_t word) { return static_cast<std::int8_t>(word); }
float float_at(std::uint32_t word) {
    float value;
    std::memcpy(&value, &word, sizeof(value));
    return value;
}
std::uint32_t bits_of(float value) {
    std::uint32_t word;
    std::memcpy(&word, &value, sizeof(word));
    return word;
}
} // namespace

extern "C" int sadx_native_gamma_gun_effects(const std::uint32_t *input, std::uint32_t *output) {
    try {
        Adapter services;
        sadx::gamma_gun_effects::GunState state;
        state.mode = byte_at(input[Mode]);
        state.laser_mode = byte_at(input[Laser]);
        state.marker = byte_at(input[Marker]);
        state.lock_count = byte_at(input[LockCount]);
        state.armed = byte_at(input[Armed]);
        state.variant = byte_at(input[Variant]);
        state.charge = float_at(input[Charge]);
        state.variant_value = input[Value];
        state.fade = float_at(input[Fade]);
        const auto sentinel = state.laser_mode;
        sadx::fidelity::Context arithmetic(static_cast<std::uint16_t>(input[ControlWord]));
        sadx::gamma_gun_effects::update(state, services, arithmetic);
        std::memset(output, 0, OutWords * sizeof(std::uint32_t));
        output[OutMode] = static_cast<std::uint8_t>(state.mode);
        output[OutLaser] = static_cast<std::uint8_t>(state.laser_mode);
        output[OutMarker] = static_cast<std::uint8_t>(state.marker);
        output[OutLock] = static_cast<std::uint8_t>(state.lock_count);
        output[OutArmed] = static_cast<std::uint8_t>(state.armed);
        output[OutVariant] = static_cast<std::uint8_t>(state.variant);
        output[OutCharge] = bits_of(state.charge);
        output[OutValue] = state.variant_value;
        output[OutFade] = bits_of(state.fade);
        return state.laser_mode == sentinel ? 1 : 0;
    } catch (...) {
        return 0;
    }
}
