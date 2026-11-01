#pragma once
#include <cstddef>

namespace sadx::story_flags {
// NOTSA: SDK StoryFlags indices into the canonical live story flag owner.
// SADXEnumsNew.h FLAG_*_PLAYABLE and FLAG_*_COMPLETE declarations.
inline constexpr std::size_t kSonicPlayable = 65;
inline constexpr std::size_t kTailsPlayable = 66;
inline constexpr std::size_t kKnucklesPlayable = 67;
inline constexpr std::size_t kAmyPlayable = 68;
inline constexpr std::size_t kGammaPlayable = 69;
inline constexpr std::size_t kBigPlayable = 70;
inline constexpr std::size_t kSuperSonicPlayable = 71;
inline constexpr std::size_t kSonicComplete = 72;
inline constexpr std::size_t kTailsComplete = 73;
inline constexpr std::size_t kKnucklesComplete = 74;
inline constexpr std::size_t kAmyComplete = 75;
inline constexpr std::size_t kGammaComplete = 76;
inline constexpr std::size_t kBigComplete = 77;
inline constexpr std::size_t kMetalSonicPlayable = 89;
} // namespace sadx::story_flags
