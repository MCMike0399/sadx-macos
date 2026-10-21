#pragma once
#include "apps/native_assets.hpp"
#include "runtime/voice_assets.hpp"
#include <set>

namespace sadx::native::app {
// NOTSA: typed imported resource identities, independent of the original executable.
inline VoiceAssets load_voice_assets(const std::filesystem::path &directory) {
    const auto source = read_metadata(directory / "voice-assets.json");
    VoiceAssets result;
    id clips = source[@"clips"];
    if (![clips isKindOfClass:NSArray.class])
        throw std::runtime_error("invalid imported voice clips");
    std::set<std::pair<std::int32_t, std::int32_t>> identities;
    for (id item in clips) {
        const auto clip = dictionary(item, "voice clip");
        VoiceClipAsset value{
            static_cast<std::int32_t>(integer(clip[@"language"], 0, 1, "voice language")),
            static_cast<std::int32_t>(integer(clip[@"voice"], 0, 2048, "voice ID")),
            string(clip[@"source_path"], "voice source path"),
            string(clip[@"pcm_path"], "voice PCM path"),
            static_cast<std::uint64_t>(integer(clip[@"frames"], 1, INT64_MAX, "voice frames"))};
        auto number = std::to_string(value.voice);
        number.insert(number.begin(), 4 - number.size(), '0');
        const auto expected = std::string("SYSTEM\\SoundData\\VOICE_") +
            (value.language == 0 ? "JP" : "US") + "\\WMA\\" + number + ".wma";
        const std::filesystem::path path(value.pcm_path);
        if (value.source_path != expected || !identities.emplace(value.language, value.voice).second)
            throw std::runtime_error("imported voice identity does not match its source path");
        if (path.empty() || path.is_absolute() || path.lexically_normal() != path ||
            *path.begin() != "voice" || !std::filesystem::is_regular_file(directory / path))
            throw std::runtime_error("missing or invalid imported voice PCM resource");
        result.clips.push_back(std::move(value));
    }
    return result;
}
} // namespace sadx::native::app
