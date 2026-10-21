#pragma once
#include "game/audio_playback.hpp"
#include <cstdint>
#include <functional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

namespace sadx::voice_playback {
// NOTSA: operations on the existing canonical native voice player. Open returns
// HRESULT-style failure; Start begins the opened clip at explicit attenuation.
// An unavailable backend throws at its resource boundary, never reports success.
struct Services {
    std::function<std::int32_t(std::string_view)> open_voice;
    std::function<void(std::int32_t)> start_voice;
    std::function<std::int32_t()> reader_state;
    std::function<void()> yield;
};
enum class Boundary { VoicePlayer, Encoding };
class Error : public std::runtime_error {
  public:
    Error(Boundary boundary, std::string message)
        : std::runtime_error(std::move(message)), boundary_(boundary) {}
    Boundary boundary() const { return boundary_; }
  private:
    Boundary boundary_;
};
// FUNCTION: SONIC 0x40ce30 PlayVoiceFile
std::int32_t play_file(std::string_view, audio::MediaState &, const std::int32_t &attenuation,
                         Services &);
} // namespace sadx::voice_playback
