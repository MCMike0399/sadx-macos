#pragma once
#include <array>
#include <cstdint>

namespace sadx {
// NOTSA: numeric load-time fields of one sound queue record, excluding identities.
struct SoundRecordInitialValues {
    std::int32_t timer = 0;
    std::uint32_t flags = 0;
    // Load-time tone is zero; the sound server later clears finished tones to -1.
    std::int32_t tone = 0, pan_angle = 0, volume = 0, pitch = 0, current_volume = 0;
};
// NOTSA: maintained sound initialization data, independent of original pointer layouts.
// Sound objects, queue identities, tasks and the listener have separate owners.
struct SoundInitialState {
    std::uint32_t paused = 0, positional_paused = 0;
    std::int32_t queue_count = 0;
    std::uint8_t frame_counter = 0;
    std::uint32_t stream_initialized = 0;
    std::array<std::uint8_t, 72> overlap{};
    std::array<SoundRecordInitialValues, 36> records{};
};
// NOTSA: load-time defaults verified against the pinned image's mapped zero fill.
SoundInitialState sound_initial_state();
} // namespace sadx
