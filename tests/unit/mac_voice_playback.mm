#include "apps/native_voice_assets.hpp"
#include "game/voice_playback.hpp"
#include "platform/mac/mac_audio_playback.hpp"
#include <cassert>
#include <chrono>
#include <iostream>
#include <thread>

int main(int argc, char **argv) {
    @autoreleasepool {
        using namespace sadx;
        assert(argc == 2);
        const std::filesystem::path root(argv[1]);
        const auto assets = native::app::load_voice_assets(root);
        assert(assets.clips.size() == 2);
        audio::MediaState state;
        state.music_open = 19;
        state.music_status = 23;
        state.shared_status = 29;
        state.music_looping = 31;
        std::string error;
        state.voice_player = audio::mac::StreamPlayer::create({}, error);
        assert(state.voice_player);
        std::uint64_t total_samples = 0, total_buffers = 0;
        for (const auto &clip : assets.clips) {
            auto *previous = state.voice_player.get();
            const auto current = [&]() -> audio::mac::StreamPlayer & {
                auto *player = dynamic_cast<audio::mac::StreamPlayer *>(state.voice_player.get());
                assert(player);
                return *player;
            };
            std::int32_t attenuation = -3300;
            voice_playback::Services services{
                [&](std::string_view path) {
                    assert(path == clip.source_path);
                    auto replacement = audio::mac::StreamPlayer::create({}, error);
                    assert(replacement && replacement.get() != previous);
                    state.voice_player = std::move(replacement);
                    assert(current().prepare_clip((root / clip.pcm_path).string(), error));
                    attenuation = -4200; // The original reads volume after the open callback.
                    return std::int32_t{0};
                },
                [&](std::int32_t value) {
                    assert(value == -4200);
                    assert(current().play(value, false, error));
                },
                [&] {
                    const auto progress = current().progress();
                    assert(progress);
                    return progress.running ? 1 : 2;
                },
                [] { std::this_thread::yield(); }
            };
            assert(voice_playback::play_file(clip.source_path, state, attenuation, services) == 1);
            const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(4);
            while (!current().progress().complete && std::chrono::steady_clock::now() < deadline)
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            const auto progress = current().progress();
            const auto stats = current().stats();
            assert(progress && progress.complete && progress.presented_frames >= clip.frames);
            assert(stats.nonzero_samples && stats.completed_buffers && !stats.device_error);
            total_samples += stats.nonzero_samples;
            total_buffers += stats.completed_buffers;
            assert(state.music_open == 19 && state.music_status == 23 &&
                   state.shared_status == 29 && state.music_looping == 31);
        }
        std::cout << "{\"voice_clips\":2,\"canonical_player_replacement\":true,\"live_volume\":true,"
                  << "\"nonzero_samples\":" << total_samples << ",\"completed_buffers\":"
                  << total_buffers << ",\"device_error\":0}\n";
    }
}
