#include "platform/mac/mac_audio.hpp"
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <thread>

// NOTSA: Native platform qualification utility; it does not claim a recovered gameplay caller.
int main(int argc, char** argv) {
    if (argc < 4) {
        std::cerr << "usage: audio-qualify capture.wav seconds imported.wav [imported.wav ...]\n";
        return 2;
    }
    const double seconds = std::strtod(argv[2], nullptr);
    if (!(seconds > 0 && seconds <= 60))
        return 2;
    sadx::audio::mac::AudioOutput output;
    std::string error;
    for (int argument = 3; argument < argc; ++argument) {
        auto clip = std::make_shared<sadx::audio::mac::AudioClip>();
        if (!sadx::audio::mac::load_clip(argv[argument], *clip, error)) {
            std::cerr << error << '\n';
            return 1;
        }
        // Qualification mixes requested tracks together; keep headroom for all of them.
        const float gain = 1.0f / static_cast<float>(argc - 3);
        if (!output.play(clip, false, gain, gain)) {
            std::cerr << "audio queue rejected clip\n";
            return 1;
        }
    }
    if (!output.start(argv[1], error)) {
        std::cerr << error << '\n';
        return 1;
    }
    std::this_thread::sleep_for(std::chrono::duration<double>(seconds));
    output.shutdown();
    const auto stats = output.stats();
    std::cout << "{\"submitted_frames\":" << stats.submitted_frames
              << ",\"nonzero_samples\":" << stats.nonzero_samples
              << ",\"completed_buffers\":" << stats.completed_buffers
              << ",\"device_error\":" << stats.device_error << "}\n";
    return stats.device_error == 0 && stats.completed_buffers > 0 && stats.nonzero_samples > 0 ? 0 : 1;
}
