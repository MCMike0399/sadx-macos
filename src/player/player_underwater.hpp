#pragma once

#include "cpu/fidelity_float.hpp"
#include "math/ninja_math.hpp"

#include <array>
#include <cstdint>

namespace sadx::player_underwater {
// The per-frame pose history and the shared underwater word. RunUnderwaterStuff
// appends one pose per player each frame and clears the word; the word's other
// role (read by the scene-position code) is not established here.
struct State {
    // GLOBAL: SONIC 0x3b36d80 PosRotBufferIndex size=8
    std::array<std::uint8_t, 8> history_index = {};
    // GLOBAL: SONIC 0x3b3ce10 PositionBuffer size=24576
    std::array<NinjaVector, 2048> positions = {};
    // GLOBAL: SONIC 0x3b36df0 RotationBuffer size=24576
    std::array<std::array<std::int32_t, 3>, 2048> rotations = {};
    // The source SomeUnderwaterThing word. Its annotated whole-owner
    // migration waits for the scene-position consumer that still models it
    // in the reference tier; this is the only native storage.
    std::int32_t underwater_word = 0;
};

// NOTSA: borrowed fields of the actual task, motion and player allocations, in
// source order. The binding supplies references; no pose or timer copy exists.
struct PlayerView {
    std::uint16_t &task_status;
    const std::uint8_t &player_slot;
    const NinjaVector &task_position;
    NinjaVector &motion_acceleration;
    float &motion_accel;
    NinjaVector &effect;
    NinjaVector &acceleration;
    std::int16_t &no_control_timer;
    std::int16_t &breath_timer;
    std::uint16_t &island_number;
    float eyes_height;
    const float &shadow_top_y;
    const std::uint32_t &shadow_prior_top_attributes;
};

// NOTSA: the recorded pose of one player-table entry, borrowed.
struct Pose {
    const NinjaVector &position;
    const std::array<std::int32_t, 3> &rotation;
};

// NOTSA: callees the product already owns return their real results. The
// breath task callback is unrecovered; the product service fails at that call.
// Until shadow computation writes the shadow fields, the product only reaches
// the bit-clear path; the water branch is integrated and oracle-covered.
struct Services {
    virtual ~Services() = default;
    // The source reads playertwp[slot] and dereferences it; a missing entry
    // fails in the service before the history index store.
    virtual Pose player_pose(std::uint8_t slot) = 0;
    // PConvertVector_P2G: rotate the player-relative vector to global space.
    virtual void convert_to_global(NinjaVector &vector) = 0;
    // CreateElementalTask(8, 0, breath callback) for the breath countdown.
    // Zero is the source null task pointer (allocation failed).
    // STUB: SONIC 0x446b10 BreathCountdownTask
    virtual std::uintptr_t create_breath_task() = 0;
    virtual void store_breath_slot(std::uintptr_t task, std::uint8_t slot) = 0;
};

// FUNCTION: SONIC 0x43b960 RecordPlayerPose
void record_position(State &state, std::uint8_t slot, const Pose &pose);

// FUNCTION: SONIC 0x446f10 RunUnderwaterStuff
void run(State &state, PlayerView &player, Services &services, fidelity::Context &arithmetic);
} // namespace sadx::player_underwater
