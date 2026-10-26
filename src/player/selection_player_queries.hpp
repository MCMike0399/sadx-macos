#pragma once

#include "engine/task.hpp"
#include <cstdint>

namespace sadx::selection_player_queries {
// NOTSA: native binding of the EV_CheckCansel leaf declared by the Gamma core.
std::int32_t event_cancel(const TaskHandle &cutscene_task);
// NOTSA: native binding of the IsTimeZero leaf declared by the Gamma core.
std::uint8_t is_time_zero(std::uint8_t frames, std::uint8_t seconds, std::uint8_t minutes);
} // namespace sadx::selection_player_queries
