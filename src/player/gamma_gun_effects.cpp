#include "player/gamma_gun_effects.hpp"

#include <cstdint>
#include <cstring>

namespace sadx::gamma_gun_effects {
namespace {
constexpr int kLastMode = 5;
constexpr float kChargeStep = 1.0f;
constexpr float kReadyCharge = 15.0f;
constexpr float kHotCharge = 30.0f;

float image_float(std::uint32_t bits) {
    float value;
    std::memcpy(&value, &bits, sizeof(value));
    return value;
}
// The fade step copied from the image, about one fifteenth.
constexpr std::uint32_t kFadeStepBits = 0x3d888889u;
const float kFadeStep = image_float(kFadeStepBits);

bool below(const fidelity::ExactFloat &value, float limit) {
    const auto comparison = value.compare(limit);
    return comparison == X87Comparison::less || comparison == X87Comparison::unordered;
}

void finish(GunState &state) {
    state.armed = 0;
    state.lock_count = 0;
}

std::uintptr_t select_lock(GunState &state, int pass, Services &services) {
    // The lock selector returns null without walking when the signed count is not positive.
    if (state.lock_count <= 0) {
        state.marker = 0;
        return 0;
    }
    return services.select_lock(pass);
}

void remember_variant(GunState &state, std::uintptr_t selected, Services &services) {
    state.variant = selected == 0 ? 0 : static_cast<std::int8_t>(services.read_variant(selected));
    state.variant_value = services.variant_word(static_cast<std::uint8_t>(state.variant));
}

fidelity::ExactFloat advance_charge(GunState &state, fidelity::Context &arithmetic) {
    auto charge = fidelity::ExactFloat(arithmetic, state.charge) + kChargeStep;
    state.charge = charge.rounded();
    return charge;
}

void cool_down(GunState &state, const fidelity::ExactFloat &charge, fidelity::Context &arithmetic) {
    const auto span = charge - kReadyCharge;
    const auto weight = span * kFadeStep;
    state.fade = (fidelity::ExactFloat(arithmetic, 1.0f) - weight).rounded();
}

void track_ready(GunState &state, const fidelity::ExactFloat &charge, Services &services) {
    if (below(charge, kReadyCharge)) {
        finish(state);
        return;
    }
    state.mode = 5;
    if (select_lock(state, 1, services) == 0) {
        finish(state);
        return;
    }
    services.open_volley(state, 0);
}

void track_hot(GunState &state, const fidelity::ExactFloat &charge, Services &services,
               fidelity::Context &arithmetic) {
    cool_down(state, charge, arithmetic);
    const auto selected = select_lock(state, 1, services);
    if (selected == 0) {
        if (below(charge, kHotCharge)) {
            finish(state);
            return;
        }
        state.charge = 0.0f;
        state.mode = 3;
        finish(state);
        return;
    }
    services.open_volley(state, selected);
}
} // namespace

void update(GunState &state, Services &services, fidelity::Context &arithmetic) {
    const auto selector = static_cast<std::uint32_t>(static_cast<std::int32_t>(state.mode));
    if (selector > kLastMode) {
        finish(state);
        return;
    }
    switch (state.mode) {
    case 0:
        state.fade = 0.0f;
        state.mode = 1;
        [[fallthrough]];
    case 1: {
        const auto selected = select_lock(state, 0, services);
        remember_variant(state, selected, services);
        if (selected == 0 || state.lock_count <= 0 || state.laser_mode != 1) {
            finish(state);
            return;
        }
        // The image stores mode 2 and the ready charge before the first task create.
        state.mode = 2;
        state.charge = 14.0f;
        services.open_volley(state, selected);
        return;
    }
    case 2:
        state.charge = 14.0f;
        services.open_volley(state, 0);
        return;
    case 3: {
        state.fade = 0.0f;
        const auto selected = select_lock(state, 1, services);
        if (selected == 0) {
            state.mode = 0;
            finish(state);
            return;
        }
        remember_variant(state, selected, services);
        state.mode = 4;
        track_ready(state, advance_charge(state, arithmetic), services);
        return;
    }
    case 4:
        track_ready(state, advance_charge(state, arithmetic), services);
        return;
    case 5:
        track_hot(state, advance_charge(state, arithmetic), services, arithmetic);
        return;
    default:
        finish(state);
        return;
    }
}
} // namespace sadx::gamma_gun_effects
