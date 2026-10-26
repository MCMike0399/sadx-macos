#include "player/selection_player_queries.hpp"
#include <cstdint>
#include <memory>

extern "C" std::uint32_t native_selection_player_queries(std::uint32_t task,
                                                       std::uint8_t frames,
                                                       std::uint8_t seconds,
                                                       std::uint8_t minutes) {
    // Reference-only identity: neither original predicate dereferences the task.
    const sadx::TaskHandle handle(std::shared_ptr<void>{},
                                 reinterpret_cast<sadx::Task *>(std::uintptr_t(task)));
    const auto event = sadx::selection_player_queries::event_cancel(handle);
    const auto zero = sadx::selection_player_queries::is_time_zero(frames, seconds, minutes);
    return std::uint32_t(event) | (std::uint32_t(zero) << 1);
}
