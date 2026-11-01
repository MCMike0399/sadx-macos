#include "guest/music_state_reference.hpp"
#include "game/game_cleanup.hpp"
#include <cstring>

namespace sadx::reference {
namespace {
constexpr std::array<std::uint32_t, 6> kTrackAddresses{
    0x912698, 0x91269c, 0x9126a8, 0x912694, 0x3b29cf8, 0x3b29cf4};

// Preserve all source bits, including signed request/history values outside the catalog.
void import_signed(const std::optional<std::uint32_t> &word, std::int32_t &value) {
    if (word)
        std::memcpy(&value, &*word, sizeof value);
}
} // namespace

void MusicTracks::import_words(const TrackWords &words) {
    for (std::size_t i = 0; i < words.size(); ++i)
        known_[i] = words[i].has_value();
    import_signed(words[0], tracks_.current_song);
    import_signed(words[1], tracks_.previous_song);
    import_signed(words[2], tracks_.real_previous_song);
    import_signed(words[3], tracks_.current_voice);
    if (words[4])
        tracks_.song_queue_state = *words[4];
    if (words[5])
        tracks_.voice_queue_state = *words[5];
}

TrackWords MusicTracks::export_words() const {
    TrackWords words{static_cast<std::uint32_t>(tracks_.current_song),
                     static_cast<std::uint32_t>(tracks_.previous_song),
                     static_cast<std::uint32_t>(tracks_.real_previous_song),
                     static_cast<std::uint32_t>(tracks_.current_voice),
                     tracks_.song_queue_state, tracks_.voice_queue_state};
    for (std::size_t i = 0; i < words.size(); ++i)
        if (!known_[i])
            words[i].reset();
    return words;
}

void MusicTracks::import_source(
    const std::function<std::optional<std::uint32_t>(std::uint32_t)> &read) {
    TrackWords words;
    for (std::size_t i = 0; i < words.size(); ++i)
        words[i] = read(kTrackAddresses[i]);
    import_words(words);
}

void MusicTracks::export_source(
    const std::function<void(std::uint32_t, std::optional<std::uint32_t>)> &write) const {
    const auto words = export_words();
    for (std::size_t i = 0; i < words.size(); ++i)
        write(kTrackAddresses[i], words[i]);
}

ResetLeafResult MusicTracks::dispatch(std::uint32_t callee,
                                      std::optional<std::uint32_t> argument,
                                      ResetLeafState &leaves, const ResetLeafNested &nested) {
    if (callee == 0x4257c0) {
        cleanup::reset_song_state(tracks_);
        known_[0] = known_[1] = known_[2] = known_[4] = true;
    } else if (callee == 0x4257e0) {
        cleanup::reset_current_voice(tracks_);
        known_[3] = known_[5] = true;
    } else {
        return run_reset_leaf(callee, argument, leaves, nested);
    }
    return {ResetLeafOutcome::completed, 0, {}, {}};
}
} // namespace sadx::reference
