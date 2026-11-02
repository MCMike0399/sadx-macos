#pragma once
#include <cstddef>
#include <cstdint>
#include <string>
#include <limits>

namespace sadx::cleanup::detail {
// The fixed seed published by Reset, independent of the previous CRT state.
inline constexpr std::uint32_t kResetRandomSeed = 3735882923U;
// NOTSA: the recovered constant-format subset of PrintDebug; percent conversions
// remain an explicit reference support limit rather than guessed varargs behavior.
inline bool format_literal_debug(const std::string &format, std::string &output) {
    if (format.find('%') != std::string::npos)
        return false;
    output = format;
    return true;
}

// NOTSA: shared algorithms use lazy access to live owners. Reference availability and
// instruction diagnostics belong to the access binding, never to these decisions.
template<class Access, class Operations>
void reset(Access &state, Operations &operations) {
    operations.resume_voice_volume();
    operations.set_depth_compare_mode(1);
    operations.stop_vibration(0);
    state.clear_frame_message_work();
    state.clear_editor_status();
    operations.set_blank_queue_mode();
    operations.reset_song_state();
    operations.reset_current_voice();
    operations.stop_all_sounds();
    operations.stop_pcm(-1);
    operations.close_media(0);
    operations.close_media(1);
    operations.setup_screen();
    operations.display_message(nullptr);
    operations.initialize_heap();
    operations.initialize_tasks();
    for (std::uint32_t level = 0; level < 10; ++level)
        operations.run_level_destructor(level);
    operations.reset_loop_state();
    if (auto *buffer = state.temporary_buffer()) {
        operations.release_buffer(buffer);
        state.clear_temporary_buffer();
    }
    // A release callback may replace this slot, including with the first identity.
    if (auto *buffer = state.secondary_buffer()) {
        operations.release_buffer(buffer);
        state.clear_secondary_buffer();
    }
    operations.enable_fog();
    state.seed_random(kResetRandomSeed);
}

// NOTSA: one record loop for native PCM owners and reference support boundaries.
template<class Access, class StopPcm>
std::int32_t stop_all_sounds(Access &state, StopPcm stop_pcm) {
    for (std::size_t index = 0; index < 36; ++index) {
        // Original order: an unavailable flags read must precede the timer store.
        const auto flags = state.sound_flags(index);
        state.clear_sound_timer(index);
        state.store_sound_flags(index, flags | 16U);
        stop_pcm(static_cast<std::int32_t>(index));
        state.clear_positional_task(index);
    }
    return 0;
}

template<class Access, class ClearList>
void reset_loop_state(Access &state, ClearList clear_list) {
    const auto present = state.list_present();
    state.clear_reset_work();
    state.clear_loop_count();
    if (present)
        clear_list();
}

template<class Access, class Release>
void release_set_files(Access &state, Release release) {
    state.print_debug("ReleaseSetFile\n");
    state.clear_set_count();
    state.clear_set_entries();
    for (std::size_t index = 0; index < 6; ++index) {
        if (auto *file = state.set_file(index)) {
            release(file);
            state.clear_set_file(index);
        }
    }
}

template<class Access, class Release>
void release_camera_files(Access &state, Release release) {
    state.print_debug("ReleaseCamFile\n");
    for (std::size_t index = 0; index < 6; ++index) {
        if (auto *file = state.camera_file(index)) {
            release(file);
            state.clear_camera_file(index);
        }
    }
}

template<class Access, class Release>
void run_level_destructor(std::uint32_t mode, Access &state, Release release) {
    if (mode == 0) {
        release_set_files(state, release);
        release_camera_files(state, release);
    } else if (mode == 5 && state.is_mission_destructor()) {
        return;
    }
    // The mission guard is a separate read; SET/CAM release can change the callback.
    state.invoke_level_destructor();
    state.clear_level_destructor();
}

// NOTSA: WMPClose policy is shared; the binding reacquires the published player
// for each operation. It must not cache a player across its stop callback.
template<class Access>
void close_media(std::uint32_t mode, Access &state) {
    if (mode == 0 && state.music_open()) {
        state.stop_music();
        state.reset_music();
        state.clear_music_open();
        state.clear_music_status();
    } else if (mode == 1 && state.voice_present()) {
        state.stop_voice();
        state.reset_voice();
    }
    state.clear_shared_status();
}

// NOTSA: remaining recovered reset leaves; all facts are read at their use.
template<class Access>
void initialize_media(Access &state) {
    if (state.audio_ready() && state.music_enabled())
        state.create_music();
    else
        state.clear_music_player();
    state.clear_music_open();
    state.clear_music_looping();
    state.clear_music_status();
    state.clear_shared_status();
    if (state.audio_ready() && state.voice_enabled())
        state.create_voice();
    else
        state.clear_voice_player();
}

template<class Access>
void resume_voice_volume(Access &state) {
    state.store_voice_volume(state.saved_voice_volume());
}

template<class Access>
void set_depth_compare_mode(std::uint32_t mode, Access &state) {
    state.store_depth_compare_mode(mode);
}

template<class Access>
void set_blank_queue_mode(Access &state) {
    state.store_queue_mode(2);
}

template<class Access>
void clear_messages(Access &state) {
    state.clear_message_count();
}

template<class Access>
std::uint32_t stop_original_vibration(std::uint32_t port, Access &state) {
    (void)port;
    state.observe_vibration_comparison();
    // Original quirk: the source never advances beyond entry zero.
    if (state.first_vibration_entry() < 3) {
        state.store_first_vibration_entry(3);
        return 0;
    }
    return std::numeric_limits<std::uint32_t>::max();
}
} // namespace sadx::cleanup::detail
