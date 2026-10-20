#pragma once
#include "game/movie.hpp"
#include "runtime/native_input.hpp"
#import <Foundation/Foundation.h>
#include <cmath>
#include <filesystem>
#include <limits>
#include <map>
#include <stdexcept>

namespace sadx::native::app {
// NOTSA: native resource metadata decoded at the application boundary; no executable images.
struct ImportedAssets {
    movie::Content movies;
    std::map<std::string, std::filesystem::path> movie_files;
    std::int32_t music_volume;
    InputConfiguration input;
};
inline NSDictionary *dictionary(id value, const char *name) {
    if (![value isKindOfClass:NSDictionary.class])
        throw std::runtime_error(std::string("invalid imported object: ") + name);
    return value;
}
inline NSArray *array(id value, std::size_t size, const char *name) {
    if (![value isKindOfClass:NSArray.class] || [value count] != size)
        throw std::runtime_error(std::string("invalid imported array: ") + name);
    return value;
}
inline std::int64_t integer(id value, std::int64_t low, std::int64_t high, const char *name) {
    if (![value isKindOfClass:NSNumber.class])
        throw std::runtime_error(std::string("invalid imported integer: ") + name);
    const auto number = [value doubleValue];
    if (!std::isfinite(number) || number < double(low) || number > double(high) ||
        std::trunc(number) != number)
        throw std::runtime_error(std::string("imported integer outside range: ") + name);
    return [value longLongValue];
}
inline std::string string(id value, const char *name) {
    if (![value isKindOfClass:NSString.class] || ![value UTF8String])
        throw std::runtime_error(std::string("invalid imported string: ") + name);
    return [value UTF8String];
}
inline NSDictionary *read_metadata(const std::filesystem::path &path) {
    const auto url = [NSURL fileURLWithPath:[NSString stringWithUTF8String:path.c_str()]];
    NSError *error = nil;
    const auto data = [NSData dataWithContentsOfURL:url options:0 error:&error];
    if (!data)
        throw std::runtime_error("missing imported metadata: " + path.string());
    id value = [NSJSONSerialization JSONObjectWithData:data options:0 error:&error];
    const auto result = dictionary(value, path.filename().c_str());
    if (integer(result[@"version"], 1, 1, "version") != 1)
        throw std::runtime_error("unsupported imported metadata version");
    return result;
}
inline ImportedAssets load_assets(const std::filesystem::path &directory) {
    ImportedAssets result{};
    const auto movies = read_metadata(directory / "movie-content.json");
    const auto clips = array(movies[@"clips"], result.movies.clips.size(), "movie clips");
    const auto playlists = array(movies[@"playlists"], result.movies.playlists.size(), "movie playlists");
    for (std::size_t index = 0; index < result.movies.clips.size(); ++index) {
        const auto clip = dictionary(clips[index], "movie clip");
        auto &destination = result.movies.clips[index];
        destination.path = string(clip[@"path"], "movie path");
        destination.mode = static_cast<std::int32_t>(integer(clip[@"mode"], INT32_MIN, INT32_MAX, "movie mode"));
        destination.fade_frame = static_cast<std::int32_t>(integer(clip[@"fade_frame"], INT32_MIN, INT32_MAX, "fade frame"));
        destination.width = static_cast<std::int16_t>(integer(clip[@"width"], 1, INT16_MAX, "movie width"));
        destination.height = static_cast<std::int16_t>(integer(clip[@"height"], 1, INT16_MAX, "movie height"));
        const auto path = directory / string(clip[@"asset_path"], "movie asset");
        if (!std::filesystem::is_regular_file(path))
            throw std::runtime_error("missing imported movie: " + path.string());
        if (!result.movie_files.emplace(destination.path, path).second)
            throw std::runtime_error("duplicate imported movie path");
        id sequence = playlists[index];
        if (![sequence isKindOfClass:NSArray.class] || [sequence count] == 0)
            throw std::runtime_error("invalid imported playlist");
        for (id item in sequence)
            result.movies.playlists[index].push_back(
                static_cast<std::int8_t>(integer(item, -2, 8, "playlist entry")));
        if (result.movies.playlists[index].back() >= 0)
            throw std::runtime_error("imported playlist has no terminating control entry");
    }
    const auto music = dictionary(movies[@"configuration"], "music configuration");
    result.music_volume = movie::music_volume(
        static_cast<std::int32_t>(integer(music[@"bgm_enabled"], INT32_MIN, INT32_MAX, "BGM enable")),
        static_cast<std::int32_t>(integer(music[@"bgm_volume_percent"], INT32_MIN, INT32_MAX, "BGM volume")));
    const auto input = read_metadata(directory / "input-configuration.json");
    RawInputConfiguration raw_input{};
    raw_input.disable_mouse = integer(input[@"disable_mouse"], 0, 1, "disable mouse") != 0;
    raw_input.keyboard_player = static_cast<std::int32_t>(integer(input[@"keyboard_player"], INT32_MIN, INT32_MAX, "keyboard player"));
    const auto keys = array(input[@"keyboard_keys"], raw_input.keys.size(), "keyboard keys");
    for (std::size_t index = 0; index < raw_input.keys.size(); ++index)
        raw_input.keys[index] = static_cast<std::int32_t>(integer(keys[index], INT32_MIN, INT32_MAX, "key binding"));
    const auto pads = array(input[@"controllers"], raw_input.pads.size(), "controllers");
    for (std::size_t index = 0; index < raw_input.pads.size(); ++index) {
        const auto pad = dictionary(pads[index], "controller configuration");
        auto &destination = raw_input.pads[index];
        destination.deadzone_left = static_cast<std::int32_t>(integer(pad[@"deadzone_left"], INT32_MIN, INT32_MAX, "left deadzone"));
        destination.deadzone_right = static_cast<std::int32_t>(integer(pad[@"deadzone_right"], INT32_MIN, INT32_MAX, "right deadzone"));
        destination.radial_left = integer(pad[@"radial_left"], 0, 1, "left radial setting") != 0;
        destination.radial_right = integer(pad[@"radial_right"], 0, 1, "right radial setting") != 0;
        destination.trigger_threshold = static_cast<std::int32_t>(integer(pad[@"trigger_threshold"], INT32_MIN, INT32_MAX, "trigger threshold"));
    }
    result.input = normalize_configuration(raw_input);
    return result;
}
} // namespace sadx::native::app
