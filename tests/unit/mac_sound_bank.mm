#include "platform/mac/mac_sound_bank.hpp"
#include "platform/mac/mac_audio_playback.hpp"
#include <cassert>
#include <chrono>
#include <fstream>
#include <iostream>
#include <thread>
using namespace sadx;
int main(int argc, char **argv) {
    assert(argc == 3);
    sound_bank::mac::Banks banks(argv[1]);
    SoundInitialState sound;
    audio::PcmState pcm(sound.stream_initialized);
    std::array<TaskHandle, 36> tasks{};
    sound_bank::ChannelBindings bindings;
    sound_bank::Channels channels{sound.records, tasks, bindings};
    sound_bank::State state;
    audio::mac::AudioOutput output(36, 256);
    std::string error;
    assert(output.start(argv[2], error));
    assert(audio::mac::bind_pcm_output(pcm, output, error));
    auto services = banks.services(channels, pcm);
    sound_bank::select(42, banks.catalog(), state, sound.stream_initialized, true, "", services);
    std::size_t samples = 0;
    for (unsigned slot : {0u, 1u, 2u, 4u}) {
        assert(state.slots[slot]);
        samples += banks.sample_count(state.slots[slot]);
    }
    assert(samples == 91 && !state.slots[5]);
    assert(banks.attempts().size() == 11);
    assert(!banks.attempts()[8].opened && !banks.attempts()[9].opened &&
           !banks.attempts()[10].opened);
    const auto first = state.slots[0];
    sound.records[0].tone = 0;
    bindings.bank_slots[0] = 0;
    auto sound_owner =
        audio::mac::make_active_sound(output, banks.sample(first, 0), true, .1f, .1f, error);
    assert(sound_owner && pcm.adopt_active(0, sound_owner));
    std::this_thread::sleep_for(std::chrono::milliseconds(150));
    assert(output.stats().nonzero_samples != 0 && output.stats().device_error == 0);
    sound_bank::select(42, banks.catalog(), state, sound.stream_initialized, true, "", services);
    assert(state.slots[0] == first && output.active_voice_count() == 1); // Same path reuses bank.
    assert(banks.attempts().size() == 14); // Empty slot is genuinely retried each source call.
    services.release_samples(0);
    assert(output.active_voice_count() == 0 && sound.records[0].tone == -1);
    services.close(first);
    bool closed = false;
    try {
        banks.sample_count(first);
    } catch (const sound_bank::Error &) {
        closed = true;
    }
    assert(closed);
    const auto stats = output.stats();
    const auto presented = output.presentation_frames(error);
    assert(presented && error.empty());
    output.shutdown();
    std::ifstream capture(argv[2], std::ios::binary);
    capture.seekg(44);
    std::uint64_t first_nonzero_frame = 0;
    float pair[2];
    bool found_sample = false;
    while (capture.read(reinterpret_cast<char *>(pair), sizeof(pair))) {
        if (pair[0] || pair[1]) {
            found_sample = true;
            break;
        }
        ++first_nonzero_frame;
    }
    assert(found_sample && first_nonzero_frame < *presented);
    std::cout << "{\"lists\":123,\"banks_opened\":4,\"samples_decoded\":" << samples
              << ",\"expected_empty_slot_attempts\":6,\"submitted_frames\":"
              << stats.submitted_frames << ",\"nonzero_samples\":" << stats.nonzero_samples
              << ",\"presented_frames\":" << *presented
              << ",\"first_nonzero_frame\":" << first_nonzero_frame
              << ",\"callbacks\":" << stats.completed_buffers
              << ",\"device_error\":" << stats.device_error << "}\n";
}
