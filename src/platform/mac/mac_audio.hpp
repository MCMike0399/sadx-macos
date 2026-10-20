#pragma once
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace sadx::audio::mac {
// NOTSA: Imported, interleaved stereo PCM at the device mix rate, 44100 Hz.
struct AudioClip {
    std::vector<float> samples;
};
bool load_clip(const std::string& path, AudioClip& clip, std::string& error);

struct AudioStats {
    std::uint64_t submitted_frames = 0;
    std::uint64_t nonzero_samples = 0;
    std::uint64_t completed_buffers = 0;
    std::int32_t device_error = 0;
};

// NOTSA: Platform PCM delivery, not a recovery of the game's voice scheduler or pan law.
// Calls may come from the session thread while AudioQueue fills on its own thread.
class AudioOutput {
public:
    explicit AudioOutput(std::size_t capacity = 36);
    ~AudioOutput();
    AudioOutput(const AudioOutput&) = delete;
    AudioOutput& operator=(const AudioOutput&) = delete;
    // A nonempty capture path receives exactly the float PCM submitted to the device.
    bool start(const std::string& capture_path, std::string& error);
    void shutdown();
    // Zero means rejected. Gains are explicit left/right channel multipliers.
    std::uint64_t play(std::shared_ptr<const AudioClip> clip, bool loop = false,
                       float left_gain = 1.0f, float right_gain = 1.0f);
    void stop(std::uint64_t token);
    void stop_all();
    AudioStats stats() const;
    // Actual output queue sample clock, relative to its start; empty/error-free means not ready.
    std::optional<std::uint64_t> presentation_frames(std::string& error) const;
    // Same mixer as the device callback. Available only while device output is stopped.
    bool render(float* stereo, std::size_t frames);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
} // namespace sadx::audio::mac
