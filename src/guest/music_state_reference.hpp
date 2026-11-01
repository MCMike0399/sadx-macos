#pragma once

#include "game/game_reset_leaves.hpp"
#include "game/music_state.hpp"
#include <array>
#include <functional>
#include <optional>

namespace sadx::reference {
// Field order matches TrackState, independent of the scattered source addresses.
using TrackWords = std::array<std::optional<std::uint32_t>, 6>;
class MusicTracks {
  public:
    music::TrackState &tracks() { return tracks_; }
    const music::TrackState &tracks() const { return tracks_; }
    void import_words(const TrackWords &);
    TrackWords export_words() const;
    void import_source(const std::function<std::optional<std::uint32_t>(std::uint32_t)> &read);
    void export_source(
        const std::function<void(std::uint32_t, std::optional<std::uint32_t>)> &write) const;
    ResetLeafResult dispatch(std::uint32_t callee, std::optional<std::uint32_t> argument,
                             ResetLeafState &, const ResetLeafNested &);

  private:
    music::TrackState tracks_;
    std::array<bool, 6> known_{};
};
} // namespace sadx::reference
