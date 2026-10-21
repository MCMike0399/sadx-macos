#include "platform/mac/mac_audio_playback.hpp"
#include <cmath>
#include <utility>

namespace sadx::audio::mac {
namespace {
// NOTSA: a real native playback/buffer lease. Destruction releases the actual
// mix token; a pending lease owns decoded sample storage without starting it.
class OutputSound final : public PcmSound {
  public:
    OutputSound(std::shared_ptr<const AudioClip> clip, AudioOutput *output, std::uint64_t token)
        : clip_(std::move(clip)), output_(output), token_(token) {}
    ~OutputSound() override {
        if (output_ && token_)
            output_->stop(token_);
    }

  private:
    std::shared_ptr<const AudioClip> clip_;
    AudioOutput *output_;
    std::uint64_t token_;
};
bool valid_clip(const std::shared_ptr<const AudioClip> &clip) {
    return clip && !clip->samples.empty() && clip->samples.size() % 2 == 0;
}
} // namespace
bool bind_pcm_output(PcmState &state, const AudioOutput &output, std::string &error) {
    error.clear();
    if (!output.ready()) {
        error = "native PCM initialization requires a successfully started audio queue";
        return false;
    }
    if (output.capacity() < kPcmVoiceCount) {
        error = "native PCM output cannot provide all 36 source channels";
        return false;
    }
    if (state.initialized != 0)
        return true;
    if (state.active_count() || state.pending_count() || output.active_voice_count()) {
        error = "native PCM cold initialization requires unassigned sound owners";
        return false;
    }
    // The queue's real worker replaces the original platform thread/events;
    // initialized remains a source lifetime guard, not a per-frame health flag.
    state.initialized = 1;
    return true;
}
std::unique_ptr<PcmSound> make_active_sound(AudioOutput &output,
                                            std::shared_ptr<const AudioClip> clip, bool loop,
                                            float left, float right, std::string &error) {
    error.clear();
    if (!output.ready()) {
        error = "native PCM playback requires a running audio queue";
        return nullptr;
    }
    if (!valid_clip(clip)) {
        error = "native PCM playback requires decoded stereo samples";
        return nullptr;
    }
    const auto token = output.play(clip, loop, left, right);
    if (token == 0) {
        error = "native PCM voice allocation rejected clip, gains or capacity";
        return nullptr;
    }
    return std::make_unique<OutputSound>(std::move(clip), &output, token);
}
std::unique_ptr<PcmSound> make_pending_sound(std::shared_ptr<const AudioClip> clip,
                                             std::string &error) {
    error.clear();
    if (!valid_clip(clip)) {
        error = "native PCM pending sound requires decoded stereo samples";
        return nullptr;
    }
    return std::make_unique<OutputSound>(std::move(clip), nullptr, 0);
}
StreamPlayer::StreamPlayer() : output_(1) {}
std::unique_ptr<StreamPlayer> StreamPlayer::create(const std::string &capture, std::string &error) {
    auto player = std::unique_ptr<StreamPlayer>(new StreamPlayer);
    player->capture_path_ = capture;
    if (!player->output_.start(capture, error))
        return nullptr;
    return player;
}
StreamPlayer::~StreamPlayer() {
    reset();
}
bool StreamPlayer::prepare_clip(const std::string &path, std::string &error) {
    error.clear();
    if (clip_) {
        error = "native stream player must be reset before another clip is prepared";
        return false;
    }
    auto clip = std::make_shared<AudioClip>();
    if (!load_clip(path, *clip, error))
        return false;
    clip_ = std::move(clip);
    return true;
}
bool StreamPlayer::play(std::int32_t attenuation, bool loop, std::string &error) {
    error.clear();
    if (!clip_ || active_) {
        error = "native stream play requires one prepared, stopped clip";
        return false;
    }
    if (attenuation < -10000 || attenuation > 0) {
        error = "native stream attenuation is outside source hundredths-dB range";
        return false;
    }
    const auto gain =
        attenuation == -10000
            ? 0.0f
            : static_cast<float>(std::pow(10.0, static_cast<double>(attenuation) / 2000.0));
    // Dedicated queue: discard old mixed buffers and prime sample 0 before start.
    output_.shutdown();
    output_.stop_all();
    const auto token = output_.play(clip_, loop, gain, gain);
    if (token == 0) {
        error = "native stream could not allocate its dedicated voice";
        return false;
    }
    auto sound = std::make_unique<OutputSound>(clip_, &output_, token);
    if (!output_.start(capture_path_, error))
        return false;
    active_ = std::move(sound);
    playback_token_ = token;
    buffer_created_ = true;
    loop_ = loop;
    started_ = true;
    return true;
}
bool StreamPlayer::set_attenuation(std::int32_t attenuation, std::string &error) {
    error.clear();
    if (!buffer_created_) {
        error = "native stream volume requires a created playback buffer";
        return false;
    }
    if (attenuation < -10000 || attenuation > 0) {
        error = "native stream attenuation is outside source hundredths-dB range";
        return false;
    }
    const auto gain = attenuation == -10000
        ? 0.0f
        : static_cast<float>(std::pow(10.0, static_cast<double>(attenuation) / 2000.0));
    // A completed voice has no remaining samples to change. The reader's
    // buffer still exists; the source supplies attenuation again when restarting.
    output_.set_gain(playback_token_, gain, gain);
    return true;
}
void StreamPlayer::stop() {
    active_.reset();
    // One reader per queue: shutdown discards already mixed samples too.
    output_.shutdown();
    started_ = false;
    playback_token_ = 0;
    buffer_created_ = false;
}
void StreamPlayer::reset() {
    stop();
    clip_.reset();
    buffer_created_ = false;
}
bool StreamPlayer::ready() const {
    return output_.ready();
}
bool StreamPlayer::clip_open() const {
    return clip_ != nullptr;
}
StreamProgress StreamPlayer::progress() const {
    StreamProgress result;
    if (!started_)
        return result;
    result.running = true;
    const auto clock = output_.presentation_frames(result.error);
    if (!result || !clock)
        return result;
    result.clock_ready = true;
    result.presented_frames = *clock;
    result.complete = !loop_ && *clock >= clip_->samples.size() / 2;
    result.running = !result.complete;
    return result;
}
AudioStats StreamPlayer::stats() const {
    return output_.stats();
}
bool create_media_players(MediaState &state, const PcmState &pcm, const AudioConfiguration &config,
                          MediaInitialization &report, std::string &error) {
    error.clear();
    report = {};
    if (state.music_player || state.voice_player) {
        error = "native media cold initialization cannot replace existing player owners";
        return false;
    }
    if (pcm.initialized != 0 && config.music_enabled) {
        report.music_attempted = true;
        state.music_player = StreamPlayer::create({}, report.music_error);
    }
    state.music_open = 0;
    state.music_looping = 0;
    state.music_status = 0;
    state.shared_status = 0;
    if (pcm.initialized != 0 && config.voice_enabled) {
        report.voice_attempted = true;
        state.voice_player = StreamPlayer::create({}, report.voice_error);
    }
    return true;
}
} // namespace sadx::audio::mac
