#pragma once
#include "platform/mac/mac_audio.hpp"
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace sadx::movie::mac {
// [platform:movie] IBasicAudio volume units: 0 = full amplitude, -10000 = silence.
std::optional<float> volume_gain(std::int32_t hundredths_db) noexcept;
// NOTSA: Native decoded movie surface. Row zero is the top row, matching the shared BGRA renderer.
struct MovieFrame {
    std::uint32_t width = 0, height = 0;
    std::uint64_t index = 0;
    double presentation_seconds = 0;
    std::vector<std::uint8_t> bgra;
};
struct MovieInfo {
    std::uint32_t width = 0, height = 0;
    std::uint64_t audio_frames = 0;
    double video_end_seconds = 0;
};
struct MovieUpdate {
    const MovieFrame* ready_frame = nullptr; // Borrowed until the next update/stop.
    bool complete = false;
    bool clock_ready = false;
    std::uint64_t presented_audio_frames = 0;
    std::string error;
    explicit operator bool() const noexcept { return error.empty(); }
};

// NOTSA: AVFoundation replaces DirectShow decoding. Game code owns the synchronous message/input/
// render/present loop and held-button skip policy. This object owns only native media resources.
class MoviePlayer {
public:
    static std::unique_ptr<MoviePlayer> open(const std::string& imported_movie, std::string& error);
    ~MoviePlayer();
    MoviePlayer(const MoviePlayer&) = delete;
    MoviePlayer& operator=(const MoviePlayer&) = delete;
    const MovieInfo& info() const noexcept;
    // A separate graph audio renderer primes PCM before device start: sample zero is known.
    bool start(std::int32_t hundredths_db, const std::string& audio_capture, std::string& error);
    MovieUpdate update();
    void stop();
    audio::mac::AudioStats audio_stats() const;

private:
    struct Impl;
    explicit MoviePlayer(std::unique_ptr<Impl>);
    std::unique_ptr<Impl> impl_;
};
} // namespace sadx::movie::mac
