#pragma once
#include "apps/native_assets.hpp"
#include "runtime/music_assets.hpp"
#include <set>

namespace sadx::native::app {
// NOTSA: import metadata becomes typed native resources before the game starts.
inline MusicAssets load_music_assets(const std::filesystem::path &directory) {
    const auto source = read_metadata(directory / "music-assets.json");
    const auto catalog = dictionary(source[@"catalog"], "music catalog");
    MusicAssets result;
    const auto songs = array(catalog[@"songs"], result.catalog.songs.size(), "music songs");
    for (std::size_t index = 0; index < result.catalog.songs.size(); ++index) {
        const auto song = dictionary(songs[index], "music song");
        auto &target = result.catalog.songs[index];
        target.name = string(song[@"name"], "music name");
        if (target.name.empty() || target.name.find_first_of("/\\.:") != std::string::npos)
            throw std::runtime_error("invalid imported music name");
        target.loop = static_cast<std::int32_t>(integer(song[@"loop"], INT32_MIN, INT32_MAX,
                                                       "music loop"));
    }
    const auto durations = [](id values, auto &target) {
        const auto items = array(values, target.size(), "voice durations");
        for (std::size_t index = 0; index < target.size(); ++index)
            target[index] = static_cast<std::uint16_t>(
                integer(items[index], 0, UINT16_MAX, "voice duration"));
    };
    durations(catalog[@"japanese_voice_durations"], result.catalog.japanese_voice_durations);
    durations(catalog[@"english_voice_durations"], result.catalog.english_voice_durations);
    const auto requests = array(catalog[@"menu_requests"], 1, "proved menu music requests");
    for (id request in requests)
        result.catalog.menu_requests.push_back(static_cast<std::int32_t>(
            integer(request, 0, result.catalog.songs.size() - 1, "menu song")));
    id clips = source[@"clips"];
    if (![clips isKindOfClass:NSArray.class])
        throw std::runtime_error("invalid imported music clips");
    std::set<std::int32_t> identities;
    for (id item in clips) {
        const auto clip = dictionary(item, "music clip");
        MusicClipAsset target{
            static_cast<std::int32_t>(
                integer(clip[@"song"], 0, result.catalog.songs.size() - 1, "clip song")),
            string(clip[@"source_path"], "music source path"),
            string(clip[@"pcm_path"], "music PCM path"),
            static_cast<std::uint64_t>(integer(clip[@"frames"], 1, INT64_MAX, "music frames"))};
        const auto expected = "SYSTEM\\SoundData\\BGM\\WMA\\" +
                              result.catalog.songs[target.song].name + ".wma";
        if (target.source_path != expected || !identities.insert(target.song).second)
            throw std::runtime_error("imported music identity does not match its catalog");
        const std::filesystem::path path(target.pcm_path);
        if (path.is_absolute() || path.lexically_normal() != path ||
            path.empty() || *path.begin() != "music" ||
            !std::filesystem::is_regular_file(directory / path))
            throw std::runtime_error("missing or invalid imported music PCM resource");
        result.clips.push_back(std::move(target));
    }
    return result;
}
} // namespace sadx::native::app
