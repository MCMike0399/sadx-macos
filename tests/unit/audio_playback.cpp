#include "game/audio_playback.hpp"
#include <cassert>
#include <functional>
#include <thread>
#include <vector>
using namespace sadx::audio;
namespace {
struct Sound final : PcmSound {
    PcmState &state;
    std::size_t slot;
    bool pending;
    std::vector<int> &trace;
    Sound(PcmState &s, std::size_t n, bool p, std::vector<int> &t)
        : state(s), slot(n), pending(p), trace(t) {}
    ~Sound() override {
        assert((pending ? state.pending(slot) : state.active(slot)) == this);
        bool unlocked = false;
        std::thread observer([&] {
            unlocked = state.active_mutex().try_lock();
            if (unlocked)
                state.active_mutex().unlock();
        });
        observer.join();
        assert(unlocked == pending);
        trace.push_back(static_cast<int>(slot * 2 + pending));
    }
};
struct Player final : StreamPlayer {
    std::vector<int> &trace;
    int marker = 1;
    std::function<void()> on_stop;
    explicit Player(std::vector<int> &t) : trace(t) {}
    void stop() override {
        trace.push_back(marker);
        if (on_stop)
            on_stop();
    }
    void reset() override {
        trace.push_back(marker + 1);
    }
};
} // namespace
int main() {
    std::uint32_t initialized = 0;
    std::vector<int> trace;
    PcmState pcm(initialized);
    for (std::size_t n = 0; n != kPcmVoiceCount; ++n) {
        std::unique_ptr<PcmSound> active = std::make_unique<Sound>(pcm, n, false, trace);
        std::unique_ptr<PcmSound> pending = std::make_unique<Sound>(pcm, n, true, trace);
        assert(pcm.adopt_active(n, active));
        assert(pcm.adopt_pending(n, pending));
        assert(!active && !pending);
    }
    assert(stop_pcm(pcm, -1));
    assert(trace.empty());
    initialized = 1;
    assert(!stop_pcm(pcm, -2));
    assert(!stop_pcm(pcm, 36));
    assert(stop_pcm(pcm, 17));
    assert((trace == std::vector<int>{34, 35}));
    assert(pcm.active_count() == 35 && pcm.pending_count() == 35);
    trace.clear();
    assert(stop_pcm(pcm, -1));
    assert(trace.size() == 70);
    std::size_t index = 0;
    for (int n = 0; n != 72; ++n) {
        if (n != 34 && n != 35)
            assert(trace[index++] == n);
    }
    assert(pcm.active_count() == 0 && pcm.pending_count() == 0);
    for (const auto mode : {0u, 1u, 2u}) {
        for (const auto open : {0u, 1u, 17u}) {
            trace.clear();
            MediaState media;
            media.music_player = std::make_unique<Player>(trace);
            media.voice_player = std::make_unique<Player>(trace);
            media.music_open = open;
            media.music_status = 13;
            media.shared_status = 19;
            media.music_looping = 23;
            assert(close_media(media, mode));
            const bool expected = (mode == 0 && open) || mode == 1;
            assert(trace == (expected ? std::vector<int>{1, 2} : std::vector<int>{}));
            assert(media.shared_status == 0 && media.music_looping == 23);
            assert(media.music_open == (mode == 0 ? 0 : open));
            assert(media.music_status == (mode == 0 && open ? 0u : 13u));
        }
    }
    trace.clear();
    MediaState replaced;
    auto old_player = std::make_unique<Player>(trace);
    auto new_player = std::make_unique<Player>(trace);
    new_player->marker = 101;
    std::unique_ptr<StreamPlayer> retained_old;
    old_player->on_stop = [&] {
        retained_old = std::move(replaced.music_player);
        replaced.music_player = std::move(new_player);
    };
    replaced.music_player = std::move(old_player);
    replaced.music_open = 1;
    assert(close_media(replaced, 0));
    assert((trace == std::vector<int>{1, 102}));
    MediaState missing;
    missing.music_open = 1;
    missing.shared_status = 9;
    assert(!close_media(missing, 0));
    assert(missing.music_open == 1 && missing.shared_status == 9);
    const auto pinned = normalize_audio_configuration({1, 30, 1, 70});
    assert(pinned.music_enabled && pinned.voice_enabled);
    assert(pinned.music_attenuation == -7400);
    assert(pinned.voice_attenuation == -3300 && pinned.saved_voice_attenuation == -3300);
    const auto disabled = normalize_audio_configuration({0, 100, 0, 100});
    assert(!disabled.music_enabled && !disabled.voice_enabled);
    assert(disabled.music_attenuation == -10000 && disabled.voice_attenuation == -300);
}
