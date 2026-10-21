#include "platform/mac/mac_audio_playback.hpp"
#include <cassert>
#include <chrono>
#include <iostream>
#include <thread>

int main(int argc, char **argv) {
    using namespace sadx::audio::mac;
    assert(argc == 2);
    std::string error;
    AudioClip imported;
    assert(load_clip(argv[1], imported, error));
    auto player = StreamPlayer::create({}, error);
    assert(player && !player->buffer_available());
    assert(player->prepare_clip(argv[1], error));
    assert(!player->buffer_available());
    assert(!player->set_attenuation(-3000, error));
    assert(player->play(-3000, false, error));
    assert(player->buffer_available());
    const auto wait = [](auto predicate, std::chrono::milliseconds timeout) {
        const auto deadline = std::chrono::steady_clock::now() + timeout;
        while (!predicate() && std::chrono::steady_clock::now() < deadline)
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        assert(predicate());
    };
    wait([&] { return player->stats().nonzero_samples > 0 &&
                      player->stats().completed_buffers >= 4; }, std::chrono::seconds(3));
    const auto before = player->progress();
    assert(before && before.running && !before.complete);
    assert(player->set_attenuation(-10000, error));
    // Allow the three queued device buffers to drain before comparing silent samples.
    std::this_thread::sleep_for(std::chrono::milliseconds(250));
    const auto muted = player->stats();
    std::this_thread::sleep_for(std::chrono::milliseconds(150));
    const auto silent = player->stats();
    assert(silent.submitted_frames > muted.submitted_frames);
    assert(silent.nonzero_samples == muted.nonzero_samples);
    assert(player->set_attenuation(-3000, error));
    wait([&] { return player->stats().nonzero_samples > silent.nonzero_samples; },
         std::chrono::seconds(2));
    const auto resumed = player->progress();
    assert(resumed && resumed.presented_frames > before.presented_frames);
    const auto duration = std::chrono::milliseconds(imported.samples.size() * 1000 / 2 / 44100);
    wait([&] {
        const auto progress = player->progress();
        assert(progress);
        return progress.complete;
    }, duration + std::chrono::seconds(3));
    const auto finished = player->progress();
    assert(finished.presented_frames >= imported.samples.size() / 2);
    assert(player->buffer_available());
    const auto stats = player->stats();
    assert(stats.nonzero_samples && stats.completed_buffers && !stats.device_error);
    player->stop();
    assert(!player->buffer_available() && player->clip_open());
    assert(player->play(-3000, false, error));
    assert(player->buffer_available());
    const auto restarted = player->progress();
    assert(restarted && restarted.running && !restarted.complete);
    assert(restarted.presented_frames < finished.presented_frames);
    player->reset();
    assert(!player->buffer_available() && !player->clip_open());
    std::cout << "{\"music_frames\":" << imported.samples.size() / 2
              << ",\"nonzero_samples\":" << stats.nonzero_samples
              << ",\"completed_buffers\":" << stats.completed_buffers
              << ",\"device_error\":" << stats.device_error
              << ",\"mute_without_restart\":true,\"eof_and_restart\":true}\n";
}
