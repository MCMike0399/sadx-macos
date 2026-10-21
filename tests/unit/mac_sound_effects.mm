#include "apps/native_sound_effects_assets.hpp"
#include "platform/mac/mac_sound_effects.hpp"
#include "platform/mac/mac_audio_playback.hpp"
#include "cpu/x87_math.hpp"
#include <cassert>
#include <chrono>
#include <iostream>
#include <thread>

int main(int argc, char **argv) {
    @autoreleasepool {
        using namespace sadx;
        assert(argc == 2);
        SoundInitialState sound;
        sound_effects::State work;
        sound_bank::ChannelBindings bindings;
        sound_effects::Queue queue{sound, bindings, work};
        sound_bank::State state;
        std::array<TaskHandle, 36> tasks{};
        audio::PcmState pcm(sound.stream_initialized);
        audio::mac::AudioOutput audio(36, 256);
        sound_bank::mac::Banks banks(argv[1]);
        std::string error;
        assert(audio.start({}, error));
        assert(audio::mac::bind_pcm_output(pcm, audio, error));
        auto bank_services = banks.services({sound.records, tasks, bindings}, pcm);
        sound_bank::select(36, banks.catalog(), state, pcm.initialized, true, "", bank_services);
        X87Context fp(0x027f);
        sound_effects::mac::Output calls(argv[1], queue, state, banks, pcm, audio, fp);
        sound_effects::select_bank_sample(0, 2, work, calls);
        assert(work.selected_sample.resource && work.selected_sample.source_size == 38190);
        calls.load_pcm(0, work.selected_sample, 2);
        assert(pcm.active_count() == 1 && audio.active_voice_count() == 0);
        assert(audio.stats().nonzero_samples == 0); // LoadPCM must not start playback.
        const auto *loaded = pcm.active(0);
        pcm.initialized = 0;
        calls.load_pcm(-1, {}, 2); // The source guard precedes all resource/index reads.
        assert(pcm.active(0) == loaded && calls.load_events() == 1);
        pcm.initialized = 1;
        bool rejected = false;
        try {
            calls.load_pcm(0, {}, 2);
        } catch (const std::runtime_error &) {
            rejected = true;
        }
        assert(rejected && pcm.active(0) == loaded && calls.load_events() == 1);
        calls.stop_pcm(0);
        const auto table = native::app::load_sound_effects_assets(argv[1]);
        const std::int32_t opponent = -1;
        const TaskHandle absent;
        sound_effects::ServerView view{queue, tasks, table, {opponent, absent}, absent};
        sound_effects::play_title_confirmation(queue);
        sound_effects::update_sound_server(view, calls);
        assert(calls.load_events() == 2 && calls.play_events() == 1);
        assert(pcm.active_count() == 1 && audio.active_voice_count() == 1);
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(3);
        while ((audio.stats().nonzero_samples == 0 || audio.stats().completed_buffers < 4) &&
               std::chrono::steady_clock::now() < deadline)
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        const auto stats = audio.stats();
        assert(stats.nonzero_samples && stats.completed_buffers >= 4 && !stats.device_error);
        for (unsigned frame = 0; frame != 120; ++frame)
            sound_effects::update_sound_server(view, calls);
        assert(pcm.active_count() == 0 && audio.active_voice_count() == 0);
        std::cout << "{\"source_cue_bytes\":38190,\"load_without_play\":true,\"play_events\":"
                  << calls.play_events() << ",\"nonzero_samples\":" << stats.nonzero_samples
                  << ",\"completed_buffers\":" << stats.completed_buffers
                  << ",\"device_error\":" << stats.device_error
                  << ",\"source_expiry_released\":true}\n";
    }
}
