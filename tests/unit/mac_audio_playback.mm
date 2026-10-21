#include "platform/mac/mac_audio_playback.hpp"
#include <cassert>
#include <chrono>
#include <fstream>
#include <iostream>
#include <thread>
using namespace sadx::audio;
using namespace sadx::audio::mac;
namespace {
void write_word(std::ostream &stream, std::uint32_t value, unsigned bytes) {
    for (unsigned byte = 0; byte != bytes; ++byte)
        stream.put(static_cast<char>(value >> (8 * byte)));
}
void write_test_clip(const std::string &path) {
    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    assert(file);
    file.write("RIFF", 4);
    write_word(file, 36 + 2048 * 8, 4);
    file.write("WAVEfmt ", 8);
    write_word(file, 16, 4);
    write_word(file, 3, 2);
    write_word(file, 2, 2);
    write_word(file, 44100, 4);
    write_word(file, 44100 * 8, 4);
    write_word(file, 8, 2);
    write_word(file, 32, 2);
    file.write("data", 4);
    write_word(file, 2048 * 8, 4);
    const float sample = 0.1f;
    for (unsigned n = 0; n != 2048 * 2; ++n)
        file.write(reinterpret_cast<const char *>(&sample), sizeof(sample));
    assert(file);
}
} // namespace
int main(int argc, char **argv) {
    assert(argc == 2);
    write_test_clip(argv[1]);
    std::uint32_t initialized = 0;
    PcmState pcm(initialized);
    std::string error;
    AudioOutput invalid(36, 0);
    assert(!invalid.start({}, error) && !invalid.ready());
    AudioOutput output(36, 256);
    assert(!bind_pcm_output(pcm, output, error));
    assert(initialized == 0);
    assert(!output.start("/nonexistent/native/audio.wav", error));
    assert(!output.ready());
    assert(output.start({}, error));
    assert(bind_pcm_output(pcm, output, error) && initialized == 1);
    auto clip = std::make_shared<AudioClip>();
    assert(load_clip(argv[1], *clip, error));
    for (std::size_t slot = 0; slot != kPcmVoiceCount; ++slot) {
        auto active = make_active_sound(output, clip, true, 0.01f, 0.01f, error);
        auto pending = make_pending_sound(clip, error);
        assert(active && pending);
        assert(pcm.adopt_active(slot, active));
        assert(pcm.adopt_pending(slot, pending));
    }
    assert(output.active_voice_count() == 36);
    assert(stop_pcm(pcm, 7));
    assert(output.active_voice_count() == 35);
    assert(stop_pcm(pcm, -1));
    assert(output.active_voice_count() == 0);
    assert(pcm.active_count() == 0 && pcm.pending_count() == 0);
    auto player = sadx::audio::mac::StreamPlayer::create({}, error);
    assert(player && player->ready());
    assert(player->prepare_clip(argv[1], error));
    assert(player->play(-3300, false, error));
    const auto first = player->progress();
    assert(first && !first.complete);
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(3);
    StreamProgress end;
    do {
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
        end = player->progress();
        assert(end);
    } while (!end.complete && std::chrono::steady_clock::now() < deadline);
    assert(end.complete && end.presented_frames >= clip->samples.size() / 2);
    assert(player->stats().nonzero_samples != 0);
    player->stop();
    assert(player->clip_open() && !player->ready());
    assert(!player->progress().complete && !player->progress().running);
    assert(player->play(-3300, true, error));
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    assert(player->progress().running && !player->progress().complete);
    player->reset();
    assert(!player->clip_open() && !player->ready());
    MediaState media;
    MediaInitialization report;
    assert(create_media_players(media, pcm, normalize_audio_configuration({1, 30, 1, 70}), report,
                                error));
    assert(report.music_attempted && report.voice_attempted);
    assert(media.music_player && media.voice_player);
    assert(close_media(media, 1));
    std::cout << "{\"source_channels\":36,\"first_clock_ready\":" << first.clock_ready
              << ",\"first_complete\":" << first.complete
              << ",\"final_presented_frames\":" << end.presented_frames
              << ",\"clip_frames\":" << clip->samples.size() / 2
              << ",\"active_pending_released\":72,\"real_reader_objects\":2}\n";
}
