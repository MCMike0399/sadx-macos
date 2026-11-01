#pragma once

#include <cstdint>
#include <memory>

namespace sadx {
class Task;
using TaskHandle = std::shared_ptr<Task>;
}

namespace sadx::cleanup {
// Shared by native scheduling and reference reset composition; not a source layout overlay.
struct TrackState {
    // GLOBAL: SONIC 0x912698 CurrentSong size=4
    std::int32_t current_song = -1;
    // GLOBAL: SONIC 0x91269c LastSong size=4
    std::int32_t previous_song = -1;
    // GLOBAL: SONIC 0x9126a8 RealLastSong size=4
    std::int32_t real_previous_song = -1;
    // GLOBAL: SONIC 0x912694 CurrentVoiceNumber size=4
    std::int32_t current_voice = -1;
    // GLOBAL: SONIC 0x3b29cf8 MusicQueueState size=4
    std::uint32_t song_queue_state = 0;
    // GLOBAL: SONIC 0x3b29cf4 VoiceQueueState size=4
    std::uint32_t voice_queue_state = 0;
};
}

namespace sadx::music {
using TrackState = cleanup::TrackState;
struct VoiceTiming {
    // GLOBAL: SONIC 0x3b29cec VoiceDuration size=4
    std::uint32_t duration = 0;
    // GLOBAL: SONIC 0x3b29cf0 VoiceStarted size=4
    std::uint32_t started = 0;
};
// NOTSA: one session owner for music requests, fallback timing and manager lifetime.
struct SchedulingState {
    TrackState tracks;
    VoiceTiming voice_timing;
    // GLOBAL: SONIC 0x3b29d00 SoundManager_ptr size=4
    TaskHandle manager;
};
} // namespace sadx::music
