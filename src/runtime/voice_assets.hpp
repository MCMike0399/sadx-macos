#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace sadx::native {
// NOTSA: imported PCM resource identity. Language0 means the source JP path;
// language1 represents the source non-JP/US path, not another game state owner.
struct VoiceClipAsset {
    std::int32_t language;
    std::int32_t voice;
    std::string source_path;
    std::string pcm_path;
    std::uint64_t frames;
};
struct VoiceAssets {
    std::vector<VoiceClipAsset> clips;
};
} // namespace sadx::native
