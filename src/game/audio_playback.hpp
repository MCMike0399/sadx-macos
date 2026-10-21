#pragma once
#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>

namespace sadx::audio {
inline constexpr std::size_t kPcmVoiceCount = 36;
// NOTSA: native ownership of the sound objects deleted by StopPCM. Backends
// release actual playback/buffer resources in the derived destructor.
class PcmSound {
  public:
    virtual ~PcmSound() = 0;
};
enum class PlaybackFailure { None, InvalidChannel, MissingMusicPlayer, MissingVoicePlayer };
struct PlaybackResult {
    PlaybackFailure failure = PlaybackFailure::None;
    explicit operator bool() const {
        return failure == PlaybackFailure::None;
    }
};
// NOTSA: canonical native PCM owners, with the initialized word borrowed from
// the session's existing sound state. Pending ownership is session-thread only.
class PcmState {
  public:
    explicit PcmState(std::uint32_t &initialized);
    ~PcmState();
    PcmState(const PcmState &) = delete;
    PcmState &operator=(const PcmState &) = delete;
    std::uint32_t &initialized;
    bool adopt_active(std::size_t channel, std::unique_ptr<PcmSound> &sound);
    bool adopt_pending(std::size_t channel, std::unique_ptr<PcmSound> &sound);
    const PcmSound *active(std::size_t channel) const;
    PcmSound *active(std::size_t channel);
    const PcmSound *pending(std::size_t channel) const;
    std::size_t active_count() const;
    std::size_t pending_count() const;
    std::recursive_mutex &active_mutex();

  private:
    friend PlaybackResult stop_pcm(PcmState &, std::int32_t);
    std::recursive_mutex active_mutex_;
    // Owned native pointers. They remain published until AFTER destruction,
    // matching source destructor callbacks; atomic reads avoid a C++ data race.
    std::array<std::atomic<PcmSound *>, kPcmVoiceCount> active_;
    std::array<PcmSound *, kPcmVoiceCount> pending_{};
};
// FUNCTION: SONIC 0x410100 StopPCM
PlaybackResult stop_pcm(PcmState &, std::int32_t channel);
// NOTSA: native implementations of the reader's stop/close resource operations.
// WMPClose ignores the original helper HRESULTs; unavailable owners still fail.
class StreamPlayer {
  public:
    virtual ~StreamPlayer() = default;
    virtual void stop() = 0;
    virtual void reset() = 0;
};
struct MediaState {
    std::unique_ptr<StreamPlayer> music_player, voice_player;
    std::uint32_t music_open = 0, music_status = 0, shared_status = 0;
    std::uint32_t music_looping = 0;
};
// FUNCTION: SONIC 0x40cff0 WMPClose
PlaybackResult close_media(MediaState &, std::uint32_t mode);
struct RawAudioConfiguration {
    std::int32_t music_enabled, music_percent, voice_enabled, voice_percent;
};
struct AudioConfiguration {
    bool music_enabled, voice_enabled;
    std::int32_t music_attenuation, saved_voice_attenuation, voice_attenuation;
};
// NOTSA: source WinMain audio configuration subset; no importer-side behavior.
AudioConfiguration normalize_audio_configuration(const RawAudioConfiguration &);
} // namespace sadx::audio
