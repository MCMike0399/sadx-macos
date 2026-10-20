#pragma once
#include "player/player_position.hpp"
#include "player/player_scene_position.hpp"

namespace sadx {
struct PlayerScenePositionConstants;
struct PlayerScenePositionRuntime;
struct PlayerSceneLandscape;
// Replace only the two mandatory scene callbacks. All captured resources must
// outlive the callbacks. Parameters, matrix owner and x87 context come from the
// position caller; the binder never owns a second mutable copy of those values.
void bind_player_scene_position(PlayerPositionCallees&,PlayerSceneLandscape&,
    const PlayerScenePositionConstants&,PlayerScenePositionRuntime&);
void bind_player_scene_position(PlayerPositionCallees&,PlayerSceneLandscape&,
    const PlayerScenePositionConstants&&,PlayerScenePositionRuntime&)=delete;
} // namespace sadx
