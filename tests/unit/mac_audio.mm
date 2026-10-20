#include "platform/mac/mac_audio.hpp"
#include <array>
#include <cassert>
#include <cstring>
#include <iostream>
#include <unistd.h>

int main(int argc, char** argv) {
    using namespace sadx::audio::mac;
    AudioOutput output(2);
    auto clip = std::make_shared<AudioClip>();
    clip->samples = {0.25f, -0.5f, 0.75f, 0.125f};
    std::array<float, 8> samples{};
    const auto first = output.play(clip);
    assert(first != 0);
    assert(output.render(samples.data(), 4));
    assert((samples == std::array<float, 8>{0.25f, -0.5f, 0.75f, 0.125f, 0, 0, 0, 0}));
    const auto loop = output.play(clip, true, 0.5f, 0);
    assert(loop != 0);
    assert(output.render(samples.data(), 4));
    assert((samples == std::array<float, 8>{0.125f, 0, 0.375f, 0, 0.125f, 0, 0.375f, 0}));
    assert(output.play(clip) != 0);
    assert(output.play(clip) == 0);
    output.stop(loop);
    assert(output.render(samples.data(), 4));
    assert(samples[0] == 0.25f && samples[1] == -0.5f);
    output.stop_all();
    assert(output.render(samples.data(), 4));
    assert((samples == std::array<float, 8>{}));
    assert(output.play(clip, true, 4, 4));
    assert(output.render(samples.data(), 1));
    assert(samples[0] == 1 && samples[1] == -1);
    output.stop(first); // An expired token must not stop the reused slot.
    assert(output.render(samples.data(), 1));
    assert(samples[0] == 1 && samples[1] == 0.5f);
    output.stop_all();
    auto malformed = std::make_shared<AudioClip>();
    malformed->samples = {1};
    assert(output.play(malformed) == 0);
    assert(output.play(nullptr) == 0);
    AudioClip decoded;
    std::string error;
    assert(!load_clip("/missing-sadx-audio.wav", decoded, error));
    assert(!error.empty());
    assert(!output.start("/missing-sadx-audio-directory/capture.wav", error));
    assert(output.render(samples.data(), 1));
    assert(!output.start("/missing-sadx-audio-directory/capture.wav", error));
    assert(error.find("cannot create audio capture") != std::string::npos);
    if (argc == 2 && std::strcmp(argv[1], "--device-retry") == 0) {
        // Exercise the real OS failure when output is unavailable (e.g. the device sandbox).
        // This also works with an available device; no injected AudioQueue implementation.
        char capture[] = "/private/tmp/sadx-audio-retry-XXXXXX";
        const int descriptor = mkstemp(capture);
        assert(descriptor >= 0);
        close(descriptor);
        const bool started = output.start(capture, error);
        if (started)
            output.shutdown();
        else {
            std::cout << "observed device failure: " << error << '\n';
            assert(output.render(samples.data(), 1));
        }
        const bool retried = output.start(capture, error);
        assert(error.find("already started") == std::string::npos);
        if (retried)
            output.shutdown();
        assert(output.render(samples.data(), 1));
        unlink(capture);
    }
    std::cout << "native audio mixer: one shot, loop, channel gains, stop, capacity and errors pass\n";
}
