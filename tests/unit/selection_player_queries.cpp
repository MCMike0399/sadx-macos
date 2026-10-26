#include "player/selection_player_queries.hpp"
#include <cassert>

int main() {
    using sadx::selection_player_queries::is_time_zero;
    assert(sadx::selection_player_queries::event_cancel({}) == 0);
    for (unsigned frames = 0; frames <= UINT8_MAX; ++frames) {
        for (unsigned seconds = 0; seconds <= UINT8_MAX; ++seconds) {
            for (unsigned minutes = 0; minutes <= UINT8_MAX; ++minutes) {
                const bool expected = frames == 0 && seconds == 0 && minutes == 0;
                assert(is_time_zero(frames, seconds, minutes) == expected);
            }
        }
    }
}
