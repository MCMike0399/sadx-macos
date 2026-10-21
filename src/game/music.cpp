#include "game/music.hpp"
#include <algorithm>

namespace sadx::music {
namespace {
// NOTSA: source formatting writes a256-byte local path buffer. Inputs exceeding
// that storage remain an explicit boundary instead of reproducing stack corruption.
std::string checked_path(std::string value) {
    if (value.size() >= 256)
        throw Error(Boundary::Path, "music manager path exceeds its source local storage");
    return value;
}
// NOTSA: bound a source song lookup to the imported catalog.
const Song &song_at(std::int32_t index, const Catalog &catalog) {
    if (index < 0 || static_cast<std::size_t>(index) >= catalog.songs.size())
        throw Error(Boundary::Song, "music request is outside the imported song catalog");
    return catalog.songs[static_cast<std::size_t>(index)];
}
// NOTSA: source wsprintf string construction without a platform formatter.
std::string song_path(std::string_view prefix, std::int32_t song, const Catalog &catalog) {
    return checked_path(std::string(prefix) + "SoundData\\BGM\\WMA\\" + song_at(song, catalog).name + ".wma");
}
// NOTSA: preserve the source signed %04d voice filename formatting.
std::string voice_path(std::string_view prefix, bool japanese, std::int32_t voice) {
    auto number = std::to_string(voice);
    if (number.size() < 4)
        number.insert(number.begin() + (voice < 0 ? 1 : 0), 4 - number.size(), '0');
    const auto directory = japanese ? "SoundData\\VOICE_JP\\WMA\\" : "SoundData\\VOICE_US\\WMA\\";
    return checked_path(std::string(prefix) + directory + number + ".wma");
}
// NOTSA: source ushort duration lookup with an explicit missing-data boundary.
std::uint32_t voice_duration(std::int32_t voice, bool japanese, const Catalog &catalog) {
    const auto &durations = japanese ? catalog.japanese_voice_durations : catalog.english_voice_durations;
    if (voice < 0 || static_cast<std::size_t>(voice) >= durations.size())
        throw Error(Boundary::Voice, "voice duration is outside the imported catalog");
    return durations[static_cast<std::size_t>(voice)];
}
// NOTSA: replace a source null-object fault with a named native resource boundary.
audio::StreamPlayer &music_player(audio::MediaState &media) {
    if (!media.music_player)
        throw Error(Boundary::MusicPlayer, "active music requires its canonical native player");
    return *media.music_player;
}
// NOTSA: the imported path subset is ASCII; other Windows code pages remain unrecovered.
void validate_reader_path(std::string_view path) {
    if (std::any_of(path.begin(), path.end(), [](unsigned char byte) { return byte > 127; }))
        throw Error(Boundary::Encoding, "unrecovered Windows code-page conversion in music path");
}
} // namespace
void play_song(std::int32_t song, cleanup::TrackState &tracks, const std::int32_t &enabled) {
    if (enabled != 0) {
        tracks.current_song = song;
        tracks.previous_song = song;
    }
}
std::uint32_t query_music_state(const cleanup::TrackState &tracks) { return tracks.song_queue_state; }
void initialize_manager(cleanup::TrackState &tracks, TaskServices &services) {
    tracks.current_song = -1;
    tracks.song_queue_state = 0;
    tracks.previous_song = -1;
    tracks.real_previous_song = -1;
    tracks.current_voice = -1;
    tracks.voice_queue_state = 0;
    services.create_manager();
}
void initialize_manager_task(const TaskHandle &task, TaskHandle &manager, TaskServices &services) {
    if (!manager) {
        manager = task;
        services.install_manager_callbacks(task);
    } else {
        services.destroy(task);
    }
}
void stop_menu_music(title_menu::MenuSoundState &menu, TaskServices &services) {
    if (menu.volume_task) {
        services.destroy(menu.volume_task);
        menu.volume_task.reset();
    }
    if (menu.music_task) {
        services.destroy(menu.music_task);
        menu.music_task.reset();
    }
    services.close_manager();
}
void request_song(std::int32_t song, title_menu::MenuSoundState &menu,
                  cleanup::TrackState &tracks, TaskServices &services) {
    stop_menu_music(menu, services);
    initialize_manager(tracks, services);
    if (song >= 0 && !menu.music_task) {
        const auto task = services.create_request();
        menu.music_task = task;
        if (task) {
            auto work = services.allocate_request(task);
            task->auxiliary_native_work = work;
            if (!work)
                throw Error(Boundary::RequestWork, "menu music CAlloc returned null before source work stores");
            work->song = song;
            work->elapsed = 0;
        }
    }
}
void request_menu_music(std::int32_t index, const Catalog &catalog, title_menu::MenuSoundState &menu,
                        cleanup::TrackState &tracks, TaskServices &services) {
    if (index < 0 || static_cast<std::size_t>(index) >= catalog.menu_requests.size())
        throw Error(Boundary::MenuRequest, "menu music index is outside the proved imported requests");
    request_song(catalog.menu_requests[static_cast<std::size_t>(index)], menu, tracks, services);
}
void update_request(const TaskHandle &task, title_menu::MenuSoundState &menu, cleanup::TrackState &tracks,
                    const std::int32_t &enabled, const std::int32_t &ready, TaskServices &services) {
    const auto work = std::static_pointer_cast<RequestWork>(task->auxiliary_native_work);
    if (!work)
        throw Error(Boundary::RequestWork, "menu music task has no native auxiliary work");
    const auto elapsed = work->elapsed;
    work->elapsed = static_cast<std::int32_t>(static_cast<std::uint32_t>(elapsed) + 1U);
    if (elapsed > 0 && ready != 0) {
        play_song(work->song, tracks, enabled);
        services.destroy(task);
        menu.music_task.reset();
    }
}
void update_manager(const TaskHandle &task, ManagerState &state, const Catalog &catalog,
                    std::string_view cd_prefix, ManagerServices &services) {
    auto &tracks = state.tracks;
    if (tracks.current_song >= 0 && tracks.current_song != tracks.real_previous_song) {
        if (services.readiness() != 0 && state.music_enabled) {
            const auto path = song_path("SYSTEM\\", tracks.current_song, catalog);
            const auto loop = song_at(tracks.current_song, catalog).loop;
            if (services.play_music(path, loop) == 0) {
                const auto fallback = song_path(cd_prefix, tracks.current_song, catalog);
                services.play_music(fallback, song_at(tracks.current_song, catalog).loop);
            }
        }
        tracks.real_previous_song = tracks.current_song;
    }
    services.restart_music();
    tracks.song_queue_state = services.music_status();
    if (tracks.current_voice >= 0) {
        const bool japanese = state.voice_language == 0;
        if (services.readiness() != 0 && state.voice_enabled) {
            const auto path = voice_path("SYSTEM\\", japanese, tracks.current_voice);
            if (services.play_voice(path) == 0)
                services.play_voice(voice_path(cd_prefix, japanese, tracks.current_voice));
        }
        state.voice_timing.duration = voice_duration(tracks.current_voice, japanese, catalog);
        state.voice_timing.started = services.milliseconds();
    }
    tracks.current_voice = -1;
    tracks.voice_queue_state = services.voice_status();
    if (tracks.voice_queue_state == 5) {
        const auto now = services.milliseconds();
        tracks.voice_queue_state = now - state.voice_timing.started < state.voice_timing.duration ? 3 : 5;
    }
    services.loop_children(task);
}
std::int32_t play_music_file(std::string_view path, std::int32_t loop, audio::MediaState &media,
                            const std::int32_t &attenuation, MediaServices &services) {
    validate_reader_path(path);
    if (!media.music_player)
        return 0;
    music_player(media).stop();
    music_player(media).reset();
    if (services.open_music(path) < 0)
        return 0;
    services.start_music(attenuation);
    media.music_looping = static_cast<std::uint32_t>(loop);
    media.music_open = 1;
    media.music_status = 3;
    while (services.music_reader_state() == 2)
        services.yield();
    return 1;
}
std::uint32_t restart_music(audio::MediaState &media, const std::int32_t &attenuation,
                            MediaServices &services) {
    if (media.music_open != 0) {
        music_player(media);
        if (services.music_reader_state() == 2) {
            if (media.music_looping == 0) {
                media.music_open = 0;
                media.music_status = 5;
                music_player(media).stop();
                music_player(media).reset();
            } else {
                music_player(media).stop();
                services.start_music(attenuation);
            }
        } else {
            music_player(media);
            if (services.music_buffer_available())
                services.set_music_buffer_volume(attenuation);
        }
    }
    return media.music_open;
}
std::uint32_t music_status(const audio::MediaState &media) { return media.music_status; }
std::uint32_t voice_status(const audio::MediaState &media, MediaServices &services) {
    if (!media.voice_player)
        return 5;
    return services.voice_reader_state() == 2 ? 5 : 3;
}
} // namespace sadx::music
