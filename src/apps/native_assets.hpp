#pragma once
#include "game/movie.hpp"
#include "game/audio_playback.hpp"
#include "runtime/native_input.hpp"
#include "runtime/character_texture_assets.hpp"
#include "runtime/title_assets.hpp"
#import <Foundation/Foundation.h>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <limits>
#include <map>
#include <stdexcept>

namespace sadx::native::app {
// NOTSA: native resource metadata decoded at the application boundary; no executable images.
struct ImportedAssets {
    movie::Content movies;
    std::map<std::string, std::filesystem::path> movie_files;
    audio::AudioConfiguration audio;
    InputConfiguration input;
    CharacterTextureAssets character_textures;
    TitleAssets title;
    std::int32_t text_language = 0, voice_language = 0;
    std::int32_t frame_rate_setting = 0;
    std::int32_t fog_emulation_setting = 0;
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
inline title_menu::MenuMode menu_mode(id value) {
    const auto name = string(value, "title menu mode");
    static const std::array<std::string_view, 14> names{
        "None", "CharacterSelect", "FileSelect", "TrialActSelect", "SoundTest",
        "Title", "TitleMenu", "OptionSelect", "Explanation", "MinigameSelect",
        "EmblemView", "InternetDemo", "TvSettings", "Puti"};
    for (std::size_t index = 0; index < names.size(); ++index)
        if (name == names[index])
            return static_cast<title_menu::MenuMode>(index);
    throw std::runtime_error("unsupported imported menu mode: " + name);
}
inline title_menu::ModeSelection menu_transition(id value) {
    const auto record = dictionary(value, "menu transition");
    return {menu_mode(record[@"previous"]), menu_mode(record[@"next"]),
            menu_mode(record[@"next_sub"])};
}
inline TitleAssets title_assets(const std::filesystem::path &directory) {
    const auto source = read_metadata(directory / "title-menu.json");
    TitleAssets result;
    id sequences = source[@"sequences"];
    if (![sequences isKindOfClass:NSArray.class] || [sequences count] == 0)
        throw std::runtime_error("missing imported title sequences");
    for (id item in sequences) {
        const auto record = dictionary(item, "title sequence");
        title_menu::SequenceData sequence;
        sequence.born_mode = menu_mode(record[@"born_mode"]);
        if (sequence.born_mode != title_menu::MenuMode::None)
            sequence.initial_transition = menu_transition(record[@"initial_transition"]);
        else if (record[@"initial_transition"] != NSNull.null)
            throw std::runtime_error("empty imported sequence contains an initial transition");
        id following = record[@"following"];
        if (![following isKindOfClass:NSArray.class])
            throw std::runtime_error("invalid imported sequence records");
        for (id entry in following) {
            const auto object = dictionary(entry, "menu object");
            const auto mode = menu_mode(object[@"mode"]);
            if (mode == title_menu::MenuMode::None)
                throw std::runtime_error("imported menu objects include a terminator");
            sequence.following.push_back({mode, menu_transition(object[@"transition"])});
        }
        if (sequence.born_mode == title_menu::MenuMode::None && !sequence.following.empty())
            throw std::runtime_error("empty imported sequence contains following objects");
        result.sequences.push_back(std::move(sequence));
    }
    const auto entries = array(source[@"entry_sequences"], result.entries.size(), "title entries");
    for (std::size_t index = 0; index < result.entries.size(); ++index)
        result.entries[index] = static_cast<std::size_t>(
            integer(entries[index], 0, result.sequences.size() - 1, "title sequence index"));
    id textures = source[@"initial_texture_ids"];
    if (![textures isKindOfClass:NSArray.class])
        throw std::runtime_error("invalid imported initial menu textures");
    for (id value in textures)
        result.initial_textures.push_back({
            static_cast<std::uint32_t>(integer(value, 0, 29, "menu texture id"))});
    const auto modes = array(source[@"mode_texture_ids"], result.mode_textures.size(),
                              "menu mode textures");
    for (std::size_t mode = 0; mode < result.mode_textures.size(); ++mode) {
        id ids = modes[mode];
        if (![ids isKindOfClass:NSArray.class])
            throw std::runtime_error("invalid imported menu texture IDs");
        for (id value in ids)
            result.mode_textures[mode].push_back({
                static_cast<std::uint32_t>(integer(value, 0, 29, "menu texture id"))});
    }
    const auto selection = dictionary(source[@"selection_data"], "menu stage selection");
    const auto read_stages = [](id value, auto &destination, bool require_nonempty) {
        const auto characters = array(value, destination.size(), "character stage lists");
        for (std::size_t character = 0; character < destination.size(); ++character) {
            id stages = characters[character];
            if (![stages isKindOfClass:NSArray.class] || [stages count] > 10 ||
                (require_nonempty && [stages count] == 0))
                throw std::runtime_error("invalid imported stage list");
            for (id item in stages) {
                const auto stage = dictionary(item, "menu stage");
                destination[character].push_back({
                    static_cast<std::uint8_t>(integer(stage[@"stage"], 0, 42, "menu stage")),
                    static_cast<std::uint8_t>(integer(stage[@"act"], 0, 255, "menu act"))});
            }
        }
    };
    read_stages(selection[@"trial_stages"], result.selection.trial_stages, true);
    read_stages(selection[@"subgame_stages"], result.selection.subgame_stages, false);
    const auto characters = array(selection[@"character_ids"], result.selection.character_ids.size(),
                                  "menu character IDs");
    for (std::size_t index = 0; index < result.selection.character_ids.size(); ++index)
        result.selection.character_ids[index] = static_cast<std::uint16_t>(
            integer(characters[index], 0, 7, "menu character ID"));
    const auto stage_ids = array(selection[@"display_stage_ids"],
                                 result.selection.display_stage_ids.size(), "display stage IDs");
    for (std::size_t index = 0; index < result.selection.display_stage_ids.size(); ++index)
        result.selection.display_stage_ids[index] = static_cast<std::uint8_t>(
            integer(stage_ids[index], 0, 255, "display stage ID"));
    const auto layout = dictionary(source[@"background_layout"], "title layout");
    const auto layout_bits = dictionary(source[@"background_layout_bits"], "title layout bits");
    using Layout = title_menu::BackgroundLayout;
    const std::array<std::pair<const char *, float Layout::*>, 9> layout_fields{{
        {"compact_logo_x", &Layout::compact_logo_x}, {"compact_logo_y", &Layout::compact_logo_y},
        {"logo_x", &Layout::logo_x}, {"logo_y", &Layout::logo_y},
        {"logo_start_x", &Layout::logo_start_x}, {"text_start_x", &Layout::text_start_x},
        {"text_step", &Layout::text_step}, {"text_x", &Layout::text_x}, {"text_y", &Layout::text_y}}};
    for (const auto &[name, member] : layout_fields) {
        const auto key = [NSString stringWithUTF8String:name];
        const auto word = static_cast<std::uint32_t>(
            integer(layout_bits[key], 0, UINT32_MAX, "layout float word"));
        if (![layout[key] isKindOfClass:NSNumber.class])
            throw std::runtime_error("invalid title layout number");
        const auto value = static_cast<float>([layout[key] doubleValue]);
        std::uint32_t parsed_word;
        std::memcpy(&parsed_word, &value, sizeof(value));
        if (!std::isfinite(value) || parsed_word != word)
            throw std::runtime_error("title layout numeric value differs from source float bits");
        result.background_layout.*member = value;
    }
    return result;
}
inline ImportedAssets load_assets(const std::filesystem::path &directory) {
    ImportedAssets result{};
    result.title = title_assets(directory);
    const auto settings = read_metadata(directory / "native-settings.json");
    result.text_language = static_cast<std::int32_t>(
        integer(settings[@"text_language"], INT32_MIN, INT32_MAX, "text language"));
    result.voice_language = static_cast<std::int32_t>(
        integer(settings[@"voice_language"], INT32_MIN, INT32_MAX, "voice language"));
    result.frame_rate_setting = static_cast<std::int32_t>(
        integer(settings[@"frame_rate_setting"], INT32_MIN, INT32_MAX, "frame rate setting"));
    result.fog_emulation_setting = static_cast<std::int32_t>(
        integer(settings[@"fog_emulation_setting"], INT32_MIN, INT32_MAX, "fog emulation setting"));
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
    const auto audio_settings = read_metadata(directory / "audio-configuration.json");
    if (string(audio_settings[@"settings_sha256"], "audio settings identity") !=
        string(movies[@"settings_sha256"], "movie settings identity"))
        throw std::runtime_error("imported movie and audio settings differ");
    result.audio = audio::normalize_audio_configuration({
        static_cast<std::int32_t>(integer(audio_settings[@"bgm_enabled"], INT32_MIN, INT32_MAX, "BGM enable")),
        static_cast<std::int32_t>(integer(audio_settings[@"bgm_volume_percent"], INT32_MIN, INT32_MAX, "BGM volume")),
        static_cast<std::int32_t>(integer(audio_settings[@"voice_enabled"], INT32_MIN, INT32_MAX, "voice enable")),
        static_cast<std::int32_t>(integer(audio_settings[@"voice_volume_percent"], INT32_MIN, INT32_MAX, "voice volume"))});
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
    const auto textures = read_metadata(directory / "character-textures.json");
    const auto characters =
        array(textures[@"characters"], result.character_textures.size(), "character textures");
    for (std::size_t index = 0; index < result.character_textures.size(); ++index) {
        const auto character = dictionary(characters[index], "character textures");
        if (integer(character[@"id"], 0, 8, "character texture id") !=
            static_cast<std::int64_t>(index))
            throw std::runtime_error("imported character texture order differs");
        id bindings = character[@"bindings"];
        if (![bindings isKindOfClass:NSArray.class] || [bindings count] == 0)
            throw std::runtime_error("missing imported character texture declarations");
        for (id entry in bindings) {
            const auto binding = dictionary(entry, "character texture binding");
            result.character_textures[index].push_back({
                string(binding[@"archive"], "character archive"),
                string(binding[@"resource_name"], "character texture resource"),
                static_cast<std::size_t>(integer(binding[@"texture_count"], 1, UINT32_MAX,
                                                  "character texture count"))});
        }
    }
    return result;
}
} // namespace sadx::native::app
