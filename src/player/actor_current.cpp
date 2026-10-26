#include "player/actor_current.hpp"

namespace sadx::actor_current {
void set_primary(State &state, TaskWorkHandle value, const Observation *observation) {
    state.primary = std::move(value);
    if (observation && observation->primary)
        observation->primary(state.primary);
}

void set_player(State &state, TaskWorkHandle value, const Observation *observation) {
    state.player = std::move(value);
    if (observation && observation->player)
        observation->player(state.player);
}

void set_super_sonic(State &state, std::uint8_t value, const Observation *observation) {
    state.super_sonic = value;
    if (observation && observation->super_sonic)
        observation->super_sonic(state.super_sonic);
}

const TaskWorkHandle &read_primary(const State &state) { return state.primary; }

const TaskWorkHandle &read_player(const State &state) { return state.player; }

std::uint8_t read_super_sonic(const State &state) { return state.super_sonic; }
} // namespace sadx::actor_current
