#pragma once

#include "engine/task_work.hpp"
#include <functional>

namespace sadx::actor_current {
// The slots describe the executing callback, not a registry's latest allocation.
struct State {
    // GLOBAL: SONIC 0x3abdf60 gpCharTwp size=4
    TaskWorkHandle primary = {};
    // GLOBAL: SONIC 0x3abdf64 gpCharPwp size=4
    TaskWorkHandle player = {};
    // GLOBAL: SONIC 0x3c55d45 SuperSonicFlag size=1
    std::uint8_t super_sonic = 0;
};

// NOTSA: optional publication receipts for reference instrumentation, after the store.
struct Observation {
    std::function<void(const TaskWorkHandle &)> primary{};
    std::function<void(const TaskWorkHandle &)> player{};
    std::function<void(std::uint8_t)> super_sonic{};
};

// NOTSA: preserve each source publication, including null and same-value stores.
void set_primary(State &, TaskWorkHandle, const Observation * = nullptr);
// NOTSA: no registry refresh; a source store alone replaces the current player.
void set_player(State &, TaskWorkHandle, const Observation * = nullptr);
// NOTSA: observe every byte store, including a transition immediately overwritten.
void set_super_sonic(State &, std::uint8_t, const Observation * = nullptr);
// NOTSA: borrow the authoritative slots without manufacturing a second owner.
const TaskWorkHandle &read_primary(const State &);
// NOTSA: read the last publication, which may still be null or an old allocation.
const TaskWorkHandle &read_player(const State &);
// NOTSA: read the source byte without boolean normalization.
std::uint8_t read_super_sonic(const State &);
} // namespace sadx::actor_current
