#include "runtime/native_underwater_binding.hpp"

#include "game/game_mode.hpp"
#include "runtime/native_display.hpp"
#include "runtime/native_player_system.hpp"

#include <cstdint>

namespace sadx::native {
namespace {
[[noreturn]] void missing(const char *boundary) { throw flow::UnrecoveredGameFlowCall(boundary); }

struct Product final : player_underwater::Services {
    PlayerSystem &players;
    Display &rendering;
    const player_initialization::PrimaryView &primary;
    Product(PlayerSystem &players, Display &rendering,
            const player_initialization::PrimaryView &primary)
        : players(players), rendering(rendering), primary(primary) {}
    player_underwater::Pose player_pose(std::uint8_t slot) override {
        if (slot >= players.registry.primary.size())
            missing("native_underwater_player_slot");
        const auto &entry = players.registry.primary[slot];
        if (!entry || !entry->position)
            missing("native_underwater_player_pose");
        return {*entry->position, entry->orientation.angles};
    }
    void convert_to_global(NinjaVector &vector) override {
        const PlayerOrientation orientation{primary.status, primary.rotation};
        if (!rendering.convert_player_to_global(orientation, vector))
            missing("native_underwater_convert");
    }
    // The breath countdown callback is unrecovered; fail at its call.
    std::uintptr_t create_breath_task() override { missing("native_underwater_breath_task"); }
    void store_breath_slot(std::uintptr_t, std::uint8_t) override {
        missing("native_underwater_breath_task");
    }
};
} // namespace

void run_underwater(PlayerSystem &players, Display &rendering, const TaskHandle &task,
                    const player_initialization::PrimaryView &primary, PlayerPhysicsState &body,
                    const player_initialization::PlayerHandle &player) {
    auto motion = player_initialization::bind_existing_motion(task);
    auto fields = player_initialization::bind_underwater_player(player);
    Product services(players, rendering, primary);
    player_underwater::PlayerView view{primary.status,
                                       primary.player_index,
                                       primary.position,
                                       motion->motion.acceleration,
                                       motion->source_field_38,
                                       body.effect,
                                       body.motion.acceleration,
                                       fields.no_control_timer,
                                       fields.breath_timer,
                                       fields.island_number,
                                       player->parameters.eyes_height,
                                       fields.shadow_top_y,
                                       fields.shadow_prior_attributes_top};
    player_underwater::run(players.underwater, view, services, rendering.arithmetic());
}
} // namespace sadx::native
