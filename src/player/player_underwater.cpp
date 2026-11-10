#include "player/player_underwater.hpp"

#include <cstdint>

namespace sadx::player_underwater {
namespace {
// Status bits RunUnderwaterStuff clears every frame (bits 3, 4, 5 and 12);
// their individual meanings are not established here.
constexpr std::uint16_t kStatusKeepMask = 0xefc7;
// Shadow top-attribute bits that mark the player as standing in water.
constexpr std::uint32_t kWaterAttributes = 0x400002;
// Frames of held breath before the countdown task spawns.
constexpr std::uint16_t kBreathLimit = 1080;
} // namespace

void record_position(State &state, std::uint8_t slot, const Pose &pose) {
    auto &index = state.history_index.at(slot);
    ++index;
    const auto entry = static_cast<unsigned>(slot) + static_cast<unsigned>(index) * 8u;
    state.positions.at(entry) = pose.position;
    state.rotations.at(entry) = pose.rotation;
}

void run(State &state, PlayerView &player, Services &services, fidelity::Context &arithmetic) {
    player.task_status = static_cast<std::uint16_t>(player.task_status & kStatusKeepMask);
    player.motion_acceleration = {0, 0, 0};
    player.effect = {0, 0, 0};
    player.acceleration = {0, 0, 0};
    player.motion_accel = 0;
    if (player.no_control_timer > 0)
        --player.no_control_timer;
    else
        player.no_control_timer = 0;
    const auto pose = services.player_pose(player.player_slot);
    record_position(state, player.player_slot, pose);
    player.island_number = 0;
    if ((player.shadow_prior_top_attributes & kWaterAttributes) == 0) {
        player.breath_timer = 0;
        state.underwater_word = 0;
        return;
    }
    NinjaVector eye{0, player.eyes_height, 0};
    services.convert_to_global(eye);
    // njAddVector: x adds destination + source, y and z add source + destination.
    eye.x = (fidelity::ExactFloat(arithmetic, eye.x) + player.task_position.x).rounded();
    eye.y = (fidelity::ExactFloat(arithmetic, player.task_position.y) + eye.y).rounded();
    eye.z = (fidelity::ExactFloat(arithmetic, player.task_position.z) + eye.z).rounded();
    const auto compared = fidelity::ExactFloat(arithmetic, player.shadow_top_y).compare(eye.y);
    if (compared == X87Comparison::less || compared == X87Comparison::unordered) {
        player.breath_timer = 0;
        state.underwater_word = 0;
        return;
    }
    player.breath_timer = static_cast<std::int16_t>(
        static_cast<std::uint16_t>(static_cast<std::uint16_t>(player.breath_timer) + 1u));
    if (static_cast<std::uint16_t>(player.breath_timer) != kBreathLimit) {
        state.underwater_word = 0;
        return;
    }
    const auto task = services.create_breath_task();
    state.underwater_word = 0;
    if (task != 0)
        services.store_breath_slot(task, player.player_slot);
}
} // namespace sadx::player_underwater
