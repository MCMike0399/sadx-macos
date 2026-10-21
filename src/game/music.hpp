#pragma once
#include "game/audio_playback.hpp"
#include "game/game_cleanup.hpp"
#include "game/title_menu.hpp"
#include <array>
#include <cstdint>
#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace sadx::music {
struct Song {
    std::string name;
    std::int32_t loop;
};
// NOTSA: typed imported source data; only proved menu-request indices are present.
struct Catalog {
    std::array<Song, 125> songs;
    std::array<std::uint16_t, 2049> japanese_voice_durations;
    std::array<std::uint16_t, 2049> english_voice_durations;
    std::vector<std::int32_t> menu_requests;
};
struct RequestWork {
    std::int32_t song = 0;
    std::int32_t elapsed = 0;
};
// NOTSA: these source BSS words track the synthetic voice-duration fallback clock.
struct VoiceTiming {
    std::uint32_t duration = 0;
    std::uint32_t started = 0;
};
struct TaskServices {
    std::function<void(const TaskHandle &)> destroy;
    // Source flags0/list1, initially executing SoundManager_Load.
    std::function<TaskHandle()> create_manager;
    std::function<void(const TaskHandle &)> install_manager_callbacks;
    // Source flags0/list4, executing the delayed menu request callback.
    std::function<TaskHandle()> create_request;
    // Source CAlloc(1,8); task auxiliary ownership is published by the core.
    std::function<std::shared_ptr<RequestWork>(const TaskHandle &)> allocate_request;
    std::function<void()> close_manager;
};
struct ManagerState {
    cleanup::TrackState &tracks;
    VoiceTiming &voice_timing;
    const bool &music_enabled;
    const bool &voice_enabled;
    const std::int32_t &voice_language;
};
struct ManagerServices {
    std::function<std::uint32_t()> readiness;
    std::function<std::int32_t(std::string_view, std::int32_t)> play_music;
    std::function<void()> restart_music;
    std::function<std::uint32_t()> music_status;
    std::function<std::int32_t(std::string_view)> play_voice;
    std::function<std::uint32_t()> voice_status;
    std::function<std::uint32_t()> milliseconds;
    std::function<void(const TaskHandle &)> loop_children;
};
// NOTSA: resource operations on the existing canonical native MediaState players.
// Open returns HRESULT-style negative failure; Start must start from the beginning
// using explicit attenuation. It must not enable platform looping: source restart
// owns that decision. An unavailable backend throws at its actual resource boundary.
struct MediaServices {
    std::function<std::int32_t(std::string_view)> open_music;
    std::function<void(std::int32_t)> start_music;
    std::function<std::int32_t()> music_reader_state;
    std::function<bool()> music_buffer_available;
    std::function<void(std::int32_t)> set_music_buffer_volume;
    std::function<std::int32_t()> voice_reader_state;
    std::function<void()> yield;
};
enum class Boundary { MenuRequest, Song, Voice, Path, RequestWork, MusicPlayer, Encoding };
class Error : public std::runtime_error {
  public:
    Error(Boundary boundary, std::string message)
        : std::runtime_error(std::move(message)), boundary_(boundary) {}
    Boundary boundary() const { return boundary_; }
  private:
    Boundary boundary_;
};
// FUNCTION: SONIC 0x425690 BGM_Play
void play_song(std::int32_t, cleanup::TrackState &, const std::int32_t &request_enabled);
// FUNCTION: SONIC 0x4256d0 FUN_004256d0
std::uint32_t query_music_state(const cleanup::TrackState &);
// FUNCTION: SONIC 0x4258b0 ADXTaskInit
void initialize_manager(cleanup::TrackState &, TaskServices &);
// FUNCTION: SONIC 0x425830 SoundManager_Load
void initialize_manager_task(const TaskHandle &, TaskHandle &manager, TaskServices &);
// FUNCTION: SONIC 0x505870 FUN_00505870
void stop_menu_music(title_menu::MenuSoundState &, TaskServices &);
// FUNCTION: SONIC 0x505900 FUN_00505900
void request_song(std::int32_t, title_menu::MenuSoundState &, cleanup::TrackState &, TaskServices &);
// FUNCTION: SONIC 0x505990 FUN_00505990
void request_menu_music(std::int32_t, const Catalog &, title_menu::MenuSoundState &,
                          cleanup::TrackState &, TaskServices &);
// FUNCTION: SONIC 0x5058c0 FUN_005058c0
void update_request(const TaskHandle &, title_menu::MenuSoundState &, cleanup::TrackState &,
                      const std::int32_t &request_enabled, const std::int32_t &textures_ready,
                      TaskServices &);
// FUNCTION: SONIC 0x4253d0 SoundManager_Main
void update_manager(const TaskHandle &, ManagerState &, const Catalog &, std::string_view cd_prefix,
                       ManagerServices &);
// FUNCTION: SONIC 0x40cd20 PlayMusicFile
std::int32_t play_music_file(std::string_view, std::int32_t loop, audio::MediaState &,
                               const std::int32_t &attenuation, MediaServices &);
// FUNCTION: SONIC 0x40cf50 WMPRestartMusic
std::uint32_t restart_music(audio::MediaState &, const std::int32_t &attenuation, MediaServices &);
// FUNCTION: SONIC 0x40cfe0 FUN_0040cfe0
std::uint32_t music_status(const audio::MediaState &);
// FUNCTION: SONIC 0x40cf20 FUN_0040cf20
std::uint32_t voice_status(const audio::MediaState &, MediaServices &);
} // namespace sadx::music
