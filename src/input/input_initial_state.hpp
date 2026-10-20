#pragma once
#include "engine/controller_values.hpp"
#include <array>
#include <cstdint>

namespace sadx::input {
// NOTSA: compatibility alias; controller values have one semantic owner in engine.
using ControllerInputValues = sadx::ControllerInputValues;
// NOTSA: raw edge storage, not physical scancodes or semantic action bindings.
struct InputEdgeValues {
    // Aggregation consumes lane zero. The other lanes retain their own values;
    // their full meaning is not established by the initialization recovery.
    std::array<std::array<std::uint8_t, 3>, 256> raw{};
    std::array<std::uint8_t, 256> accumulated{};
    std::uint8_t special = 0, accumulated_special = 0;
    std::array<std::uint8_t, 2> clear_prefix{};
    std::uint8_t cleared_neighbor = 0;
};
// NOTSA: load-time input data for the pinned game and Input mod, before startup.
// Name/extension/info resources and controller discovery have separate owners.
struct InputInitialState {
    InputEdgeValues edges{};
    ControllerInputValues controller_template{};
    std::array<ControllerInputValues, 4> engine_raw{};
    std::array<ControllerInputValues, 8> mod_raw{};
    std::array<ControllerInputValues, 8> dream_pads{};
};
// NOTSA: recovered load-image state, not the post-poll invariant not_held == ~held.
InputInitialState input_initial_state();
} // namespace sadx::input
