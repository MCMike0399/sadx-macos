#include "platform/mac/mac_audio_playback.hpp"
#include <cassert>
#include <chrono>
#include <fstream>
#include <iostream>
#include <iterator>
#include <thread>
using namespace sadx::audio::mac;
int main(int argc, char **argv) {
    assert(argc == 2);
    auto clip = std::make_shared<AudioClip>();
    clip->samples.assign(44100 * 2, 0.001f);
    AudioOutput output(36, 256);
    std::uint32_t initialized = 0;
    sadx::audio::PcmState pcm(initialized);
    std::string error;
    assert(output.start(argv[1], error));
    assert(bind_pcm_output(pcm, output, error));
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    const auto before_play = output.presentation_frames(error);
    assert(before_play && error.empty());
    for (std::size_t slot = 0; slot != sadx::audio::kPcmVoiceCount; ++slot) {
        auto sound = make_active_sound(output, clip, true, 1, 1, error);
        assert(sound && pcm.adopt_active(slot, sound));
    }
    assert(output.active_voice_count() == 36);
    const auto after_play = output.presentation_frames(error);
    assert(after_play && error.empty());
    std::this_thread::sleep_for(std::chrono::milliseconds(3000));
    const auto before = output.presentation_frames(error);
    assert(before && error.empty());
    assert(stop_pcm(pcm, -1));
    assert(output.active_voice_count() == 0);
    const auto after = output.presentation_frames(error);
    assert(after && error.empty());
    std::this_thread::sleep_for(std::chrono::milliseconds(180));
    const auto stats = output.stats();
    assert(stats.device_error == 0 && stats.nonzero_samples != 0);
    output.shutdown();
    std::ifstream file(argv[1], std::ios::binary);
    file.seekg(44);
    std::uint64_t frames = 0, first = 0, last = 0;
    float pair[2];
    while (file.read(reinterpret_cast<char *>(pair), sizeof(pair))) {
        if (pair[0] || pair[1]) {
            if (last == 0)
                first = frames;
            last = frames + 1;
        }
        ++frames;
    }
    std::cout << "{\"before_play_presented\":" << *before_play
              << ",\"after_play_presented\":" << *after_play << ",\"first_nonzero_frame\":" << first
              << ",\"play_latency_frames_upper_bound\":"
              << (first > *before_play ? first - *before_play : 0)
              << ",\"before_stop_presented\":" << *before << ",\"after_stop_presented\":" << *after
              << ",\"last_nonzero_frame_exclusive\":" << last
              << ",\"buffered_tail_frames_upper_bound\":" << (last > *before ? last - *before : 0)
              << ",\"source_voices\":36,\"buffer_frames\":256,\"device_error\":"
              << stats.device_error << ",\"completed_buffers\":" << stats.completed_buffers
              << ",\"nonzero_samples\":" << stats.nonzero_samples
              << ",\"sample_rate\":44100,\"capture_frames\":" << frames << "}\n";
}
