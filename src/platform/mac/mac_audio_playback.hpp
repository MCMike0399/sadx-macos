#pragma once
#include "game/audio_playback.hpp"
#include "platform/mac/mac_audio.hpp"
#include <memory>
#include <string>

namespace sadx::audio::mac {
// NOTSA: bind the source initialized word only to a successfully started native
// queue/worker with room for all 36 PCM channels. No Windows handles are invented.
bool bind_pcm_output(PcmState &, const AudioOutput &, std::string &error);
std::unique_ptr<PcmSound> make_active_sound(AudioOutput &, std::shared_ptr<const AudioClip>,
                                            bool loop, float left_gain, float right_gain,
                                            std::string &error);
std::unique_ptr<PcmSound> make_pending_sound(std::shared_ptr<const AudioClip>, std::string &error);
struct StreamProgress {
    bool running = false, complete = false, clock_ready = false;
    std::uint64_t presented_frames = 0;
    std::string error;
    explicit operator bool() const {
        return error.empty();
    }
};
// NOTSA: imported-PCM implementation of the source reader's resource lifecycle.
// It owns a separate queue, as music/voice readers are outside the 36 PCM table.
class StreamPlayer final : public audio::StreamPlayer {
  public:
    static std::unique_ptr<StreamPlayer> create(const std::string &capture, std::string &error);
    ~StreamPlayer() override;
    bool prepare_clip(const std::string &imported_path, std::string &error);
    bool play(std::int32_t attenuation, bool loop, std::string &error);
    bool set_attenuation(std::int32_t attenuation, std::string &error);
    // NOTSA: native mixer preference stays independent of source attenuation and reader state.
    bool set_output_gain(float gain) { return output_.set_output_gain(gain); }
    void stop() override;
    void reset() override;
    bool ready() const;
    bool clip_open() const;
    bool buffer_available() const { return buffer_created_; }
    StreamProgress progress() const;
    AudioStats stats() const;

  private:
    StreamPlayer();
    AudioOutput output_;
    std::shared_ptr<const AudioClip> clip_;
    std::unique_ptr<PcmSound> active_;
    std::string capture_path_;
    bool started_ = false, loop_ = false;
    bool buffer_created_ = false;
    std::uint64_t playback_token_ = 0;
};
struct MediaInitialization {
    bool music_attempted = false, voice_attempted = false;
    std::string music_error, voice_error;
};
// NOTSA: native cold-start binding of WMPInit's observable player/flag state.
// Existing player ownership is an explicit unsupported reinitialization case;
// source cue scheduling/reopening remains with its recovered game callers.
bool create_media_players(MediaState &, const PcmState &, const AudioConfiguration &,
                          MediaInitialization &, std::string &error);
} // namespace sadx::audio::mac
