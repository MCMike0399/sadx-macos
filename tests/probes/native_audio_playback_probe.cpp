#include "game/audio_playback.hpp"
#include <algorithm>
#include <vector>
using namespace sadx::audio;
namespace {
struct Sound final : PcmSound {
    std::vector<unsigned> &trace;
    unsigned value;
    Sound(std::vector<unsigned> &t, unsigned v) : trace(t), value(v) {}
    ~Sound() override {
        trace.push_back(value);
    }
};
struct Player final : StreamPlayer {
    std::vector<unsigned> &trace;
    unsigned value;
    Player(std::vector<unsigned> &t, unsigned v) : trace(t), value(v) {}
    void stop() override {
        trace.push_back(value);
    }
    void reset() override {
        trace.push_back(value + 1);
    }
};
} // namespace
extern "C" int audio_pcm_probe(unsigned initialized, int channel, const unsigned *masks,
                               unsigned *out) {
    std::vector<unsigned> trace;
    PcmState state(initialized);
    for (unsigned n = 0; n != kPcmVoiceCount; ++n) {
        if (masks[n] & 1) {
            std::unique_ptr<PcmSound> sound = std::make_unique<Sound>(trace, n * 2);
            state.adopt_active(n, sound);
        }
        if (masks[n] & 2) {
            std::unique_ptr<PcmSound> sound = std::make_unique<Sound>(trace, n * 2 + 1);
            state.adopt_pending(n, sound);
        }
    }
    if (!stop_pcm(state, channel))
        return 0;
    out[0] = static_cast<unsigned>(trace.size());
    std::copy(trace.begin(), trace.end(), out + 1);
    for (unsigned n = 0; n != kPcmVoiceCount; ++n)
        out[73 + n] = (state.active(n) ? 1u : 0u) | (state.pending(n) ? 2u : 0u);
    return 1;
}
extern "C" int audio_media_probe(const unsigned *in, unsigned *out) {
    std::vector<unsigned> trace;
    MediaState state;
    if (in[1])
        state.music_player = std::make_unique<Player>(trace, 10);
    if (in[2])
        state.voice_player = std::make_unique<Player>(trace, 20);
    state.music_open = in[3];
    state.music_status = in[4];
    state.shared_status = in[5];
    state.music_looping = in[6];
    if (!close_media(state, in[0]))
        return 0;
    out[0] = static_cast<unsigned>(trace.size());
    std::copy(trace.begin(), trace.end(), out + 1);
    out[3] = state.music_open;
    out[4] = state.music_status;
    out[5] = state.shared_status;
    out[6] = state.music_looping;
    return 1;
}
extern "C" void audio_config_probe(const int *in, int *out) {
    const auto config = normalize_audio_configuration({in[0], in[1], in[2], in[3]});
    out[0] = config.music_enabled;
    out[1] = config.music_attenuation;
    out[2] = config.voice_enabled;
    out[3] = config.voice_attenuation;
    out[4] = config.saved_voice_attenuation;
}
