#pragma once
// Reference-only forced include for frozen Sonic fixture translation units.
// Do not inject into any production translation unit.
#include "player/sonic_player.hpp"

namespace sadx::reference {
template <typename T> struct PublicationReceipt {
    const T &value;
    bool written = false;
    bool has_value() const { return written; }
    explicit operator bool() const { return written; }
    const T &operator*() const { return value; }
    T value_or(T fallback) const { return written ? value : fallback; }
    bool operator==(const T &other) const { return written && value == other; }
};
struct SonicPlayerFixtureGlobals : SonicPlayerGlobals {
    PublicationReceipt<TaskWorkHandle> current_task_at_3abdf60{current.primary, false};
    PublicationReceipt<TaskWorkHandle> current_player_at_3abdf64{current.player, false};
    PublicationReceipt<std::uint8_t> range_at_3c55d45{current.super_sonic, false};
    actor_current::Observation receipts{
        [this](const TaskWorkHandle &) { current_task_at_3abdf60.written = true; },
        [this](const TaskWorkHandle &) { current_player_at_3abdf64.written = true; },
        [this](std::uint8_t) { range_at_3c55d45.written = true; }};
    SonicPlayerFixtureGlobals() { current_observation = &receipts; }
    SonicPlayerFixtureGlobals(const SonicPlayerFixtureGlobals &) = delete;
    SonicPlayerFixtureGlobals &operator=(const SonicPlayerFixtureGlobals &) = delete;
};
} // namespace sadx::reference
#define SonicPlayerGlobals reference::SonicPlayerFixtureGlobals
