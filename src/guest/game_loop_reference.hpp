#pragma once
#include "game/game_loop.hpp"
#include "guest/reset_control_reference.hpp"
namespace sadx {
// Globals read/written by the local body of GameLoop 0x40cad0. Callees own the
// rest of the game state; their effects arrive through the borrowed owners.
struct GameLoopState {
    std::optional<std::uint32_t> word_at_3abdcf8,exit_at_3abdf7c,focus_at_8929d4;
    std::optional<std::uint8_t> resync_at_3abdf68;
    // Read-only here; borrowed from their owners (queue consumer state 909eb4,
    // sprite queue runtime 3abdcfe). Null is an unknown fact at the read.
    const std::optional<std::uint8_t>* special_flags_at_909eb4=nullptr;
    const std::optional<std::uint8_t>* fog_flag_at_3abdcfe=nullptr;
    std::optional<std::uint32_t> fog_range_at_3abdca0,fog_range_at_3abdca4,fog_range_at_3abdc70,fog_range_at_3abdc74;
    std::optional<std::uint8_t> request_at_3b291ad,counter_at_3b291b2,pending_at_3abdf76,pending_at_3abdf79;
    std::optional<std::int32_t> save_phase_at_3b291a4;
    reference::FrameMessageFact frame_message{};
    std::optional<std::uint32_t> mode_at_3b28114,frames_at_3abdf58,frames_at_3abdf5c,word_at_3c85804;
    std::optional<std::uint16_t> game_mode_at_3b22de4;
};
}
