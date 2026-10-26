#include "player/selection_player_queries.hpp"

namespace sadx::selection_player_queries {
// NOTSA: source-leaf binding; original callee provenance belongs to gamma_selection_core.hpp.
std::int32_t event_cancel(const TaskHandle &cutscene_task) {
    return cutscene_task != nullptr;
}

// NOTSA: source-leaf binding; read the three bootstrap timer bytes in source order.
std::uint8_t is_time_zero(std::uint8_t frames, std::uint8_t seconds, std::uint8_t minutes) {
    return frames == 0 && seconds == 0 && minutes == 0;
}
} // namespace sadx::selection_player_queries
