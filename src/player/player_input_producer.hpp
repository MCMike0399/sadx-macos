#pragma once

#include "game/game_controller.hpp"
#include "player/player_input.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>

namespace sadx {

struct PlayerInputProducerConstants {
    float positive_deadzone;   // 0x7dccf4
    float negative_deadzone;   // 0x7dccf0
    float magnitude_scale;     // 0x7dcce0
    float magnitude_cap;       // 0x7dcb10
    double angle_units;        // 0x7dccd0
    double positive_factor;    // 0x7dccc8
    double negative_factor;    // 0x7dcce8
};

// Exact little-endian field order above: four f32 words, three f64 words.
// The executable/table owner supplies bytes and provenance; no hidden defaults.
std::optional<PlayerInputProducerConstants> decode_player_input_producer_constants(
    const std::uint8_t* bytes, std::size_t size);

struct PlayerInputPadAxes { std::int16_t x, y; };
struct PlayerInputCamera {
    // false means an observed null camera pointer, not unavailable camera state.
    bool present;
    std::optional<std::int32_t> yaw;
};
struct InputMod261Pad {
    std::optional<std::uint8_t> connected;
    std::optional<std::uint8_t> keyboard_allowed;
    std::optional<PlayerInputPadAxes> axes;
    std::optional<float> normalized_magnitude;
};
struct PlayerInputProducerRuntime {
    // Borrowed live ControllerPointers[0..3]; the original body reads signed16
    // +0x1c/+0x1e only for slots0/1. These records need not equal DreamPad's
    // source pad fields below (demo/other owners may differ).
    const GameLogicControllers* raw_controllers = nullptr;
    std::optional<PlayerInputCamera> camera;
    std::optional<std::uint8_t> control_enabled;
    // Input2.6.1 patches the original controller-enabled operand to this same
    // effective table. Original tests ==1; updated hook tests !=0.
    std::array<std::optional<std::uint8_t>, 8> controller_enabled;
    std::array<std::optional<InputMod261Pad>, 8> input_mod_pads;
};
struct PlayerInputProducedState {
    std::array<std::optional<NormalizedPlayerInput>, 8> raw_unused; // 0x3b0e368
    std::array<std::optional<NormalizedPlayerInput>, 8> normalized; // 0x3b0e7a0
};
enum class PlayerInputProducerFailure {
    none, unavailable_raw_controller, null_raw_controller, unavailable_raw_axis,
    unavailable_camera_presence,
    unavailable_camera_yaw, unavailable_control_enabled,
    unavailable_controller_enabled, unavailable_input_mod_pad,
    unavailable_connection, unavailable_keyboard_allowed,
    unavailable_input_mod_axes, unavailable_input_mod_magnitude,
};
struct PlayerInputProducerResult {
    PlayerInputProducerFailure failure = PlayerInputProducerFailure::none;
    std::uint32_t source_boundary = 0;
    std::uint8_t slot = 0;
    explicit operator bool() const noexcept { return failure == PlayerInputProducerFailure::none; }
};

// Full original WriteAnalogs0x40f170 with the explicitly named Bochs P5/P6
// FPATAN model and real CRTftol. Hardware Intel Ultra agreement remains
// unverified. Earlier slots/stores and x87 flags survive later failures.
PlayerInputProducerResult write_analogs_bochs_p5_p6(
    const PlayerInputProducerRuntime& runtime, const PlayerInputProducerConstants& constants,
    PlayerInputProducedState& state, X87Context& context);

// Literal actual Input2.6.1 hook body0x10004b00..4b6b (hook entry4b70), with
// caller-provided prepared pad/magnitude state. This is integer logic/MOV,
// not a controller deadzone, keyboard normalization or an x87 calculation.
PlayerInputProducerResult override_input_mod_261_magnitudes(
    const PlayerInputProducerRuntime& runtime, PlayerInputProducedState& state);

// Updated profile owner: original body completes before its return-hook runs.
PlayerInputProducerResult produce_input_mod_261_bochs_p5_p6(
    const PlayerInputProducerRuntime& runtime, const PlayerInputProducerConstants& constants,
    PlayerInputProducedState& state, X87Context& context);

} // namespace sadx
