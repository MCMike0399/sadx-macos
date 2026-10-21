#include "game/audio_playback.hpp"
#include <algorithm>

namespace sadx::audio {
PcmSound::~PcmSound() = default;
PcmState::PcmState(std::uint32_t &source_initialized) : initialized(source_initialized) {
    for (auto &sound : active_)
        sound.store(nullptr);
}
PcmState::~PcmState() {
    // NOTSA: native owner teardown, independent of the game's initialized guard.
    for (auto &sound : active_)
        delete sound.exchange(nullptr);
    for (auto *sound : pending_)
        delete sound;
}
bool PcmState::adopt_active(std::size_t channel, std::unique_ptr<PcmSound> &sound) {
    if (channel >= kPcmVoiceCount || !sound)
        return false;
    std::lock_guard lock(active_mutex_);
    if (active_[channel].load())
        return false;
    active_[channel].store(sound.release());
    return true;
}
bool PcmState::adopt_pending(std::size_t channel, std::unique_ptr<PcmSound> &sound) {
    if (channel >= kPcmVoiceCount || !sound || pending_[channel])
        return false;
    pending_[channel] = sound.release();
    return true;
}
const PcmSound *PcmState::active(std::size_t channel) const {
    return active_.at(channel).load();
}
PcmSound *PcmState::active(std::size_t channel) {
    return active_.at(channel).load();
}
const PcmSound *PcmState::pending(std::size_t channel) const {
    return pending_.at(channel);
}
std::size_t PcmState::active_count() const {
    return std::count_if(active_.begin(), active_.end(), [](const auto &sound) {
        return sound.load() != nullptr;
    });
}
std::size_t PcmState::pending_count() const {
    return std::count_if(pending_.begin(), pending_.end(), [](const auto *sound) {
        return sound != nullptr;
    });
}
std::recursive_mutex &PcmState::active_mutex() {
    return active_mutex_;
}
PlaybackResult stop_pcm(PcmState &state, std::int32_t channel) {
    if (state.initialized == 0)
        return {};
    if (channel < -1 || channel >= static_cast<std::int32_t>(kPcmVoiceCount))
        return {PlaybackFailure::InvalidChannel};
    const auto first = channel == -1 ? 0 : channel;
    const auto end = channel == -1 ? static_cast<std::int32_t>(kPcmVoiceCount) : channel + 1;
    for (auto index = first; index < end; ++index) {
        const auto slot = static_cast<std::size_t>(index);
        if (state.active_[slot].load()) {
            std::lock_guard lock(state.active_mutex_);
            if (auto *sound = state.active_[slot].load()) {
                delete sound;
                state.active_[slot].store(nullptr);
            }
        }
        if (auto *sound = state.pending_[slot]) {
            delete sound;
            state.pending_[slot] = nullptr;
        }
    }
    return {};
}
PlaybackResult close_media(MediaState &state, std::uint32_t mode) {
    if (mode == 0 && state.music_open != 0) {
        if (!state.music_player)
            return {PlaybackFailure::MissingMusicPlayer};
        state.music_player->stop();
        // The source reloads the canonical player after the stop callback.
        if (!state.music_player)
            return {PlaybackFailure::MissingMusicPlayer};
        state.music_player->reset();
        state.music_open = 0;
        state.music_status = 0;
    } else if (mode == 1 && state.voice_player) {
        state.voice_player->stop();
        if (!state.voice_player)
            return {PlaybackFailure::MissingVoicePlayer};
        state.voice_player->reset();
    }
    state.shared_status = 0;
    return {};
}
AudioConfiguration normalize_audio_configuration(const RawAudioConfiguration &raw) {
    const auto music_percent = std::clamp(raw.music_percent, 0, 100);
    const auto voice_percent = std::clamp(raw.voice_percent, 0, 100);
    const bool music_enabled = std::clamp(raw.music_enabled, 0, 1) != 0 && music_percent != 0;
    const bool voice_enabled = std::clamp(raw.voice_enabled, 0, 1) != 0 && voice_percent != 0;
    const auto voice_volume = (voice_percent * 97 / 100 - 100) * 100;
    const auto music_volume = music_enabled ? (music_percent * 89 / 100 - 100) * 100 : -10000;
    // Disabled voice does not force the saved/current attenuation to mute.
    return {music_enabled, voice_enabled, music_volume, voice_volume, voice_volume};
}
} // namespace sadx::audio
