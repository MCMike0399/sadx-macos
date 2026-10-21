#include "platform/mac/mac_audio.hpp"
#include <array>
#include <cassert>
#include <chrono>
#include <fstream>
#include <iostream>
#include <limits>
#include <thread>
#include <unistd.h>

int main() {
    using namespace sadx::audio::mac;
    AudioOutput output(1);
    auto clip = std::make_shared<AudioClip>();
    clip->samples = {0.25f, -0.5f, 0.75f, 0.125f};
    const auto token = output.play(clip, true);
    assert(token);
    std::array<float, 2> samples{};
    assert(output.render(samples.data(), 1));
    assert(samples[0] == 0.25f && samples[1] == -0.5f); // Default faithful mix.
    assert(output.set_output_gain(0.5f));
    assert(output.render(samples.data(), 1));
    assert(samples[0] == 0.375f && samples[1] == 0.0625f); // Cursor did not restart.
    assert(output.set_gain(token, 0.25f, 0.5f));
    assert(!output.set_output_gain(-1) && !output.set_output_gain(8.01f));
    assert(!output.set_output_gain(std::numeric_limits<float>::quiet_NaN()));
    assert(output.render(samples.data(), 1));
    assert(samples[0] == 0.03125f && samples[1] == -0.125f);
    assert(output.set_output_gain(0));
    assert(output.render(samples.data(), 1) && samples[0] == 0 && samples[1] == 0);
    assert(output.set_output_gain(8));
    assert(output.set_gain(token, 0.01f, 0.01f));
    assert(output.render(samples.data(), 1));
    assert(samples[0] == 0.25f * 0.01f * 8 && samples[1] == -0.5f * 0.01f * 8);

    // Actual AudioQueue capture uses the same mixer and includes its applied native gain.
    char filename[] = "/private/tmp/sadx-native-audio-gain-XXXXXX";
    const int descriptor = mkstemp(filename);
    assert(descriptor >= 0);
    close(descriptor);
    AudioOutput device(1, 256);
    auto constant = std::make_shared<AudioClip>();
    constant->samples = {0.5f, -0.25f};
    assert(device.set_output_gain(0.4f) && device.play(constant, true));
    std::string error;
    assert(device.start(filename, error));
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(3);
    while (!device.stats().completed_buffers && std::chrono::steady_clock::now() < deadline)
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    device.shutdown();
    const auto stats = device.stats();
    assert(stats.completed_buffers && stats.nonzero_samples && !stats.device_error);
    std::ifstream capture(filename, std::ios::binary);
    capture.seekg(44);
    std::size_t frames = 0;
    while (capture.read(reinterpret_cast<char *>(samples.data()), sizeof(samples))) {
        assert(samples[0] == 0.2f && samples[1] == -0.1f);
        ++frames;
    }
    capture.close();
    unlink(filename);
    assert(frames);
    std::cout << "Native mixer gain preserves source gains/cursors; actual device samples scaled40percent\n";
}
