#pragma once

#include "player/player_scene_broadphase.hpp"
#include "player/player_scene_decode.hpp"
#include "player/player_task.hpp"

namespace sadx {
// Borrow live task owners. Absent optional = unknown selector; a present
// view with nullptr = observed null source task pointer.
struct SceneTaskReadView { const PlayerTaskState* task; };
struct SceneRegisteredLandEntry {
    SceneLandEntry land;
    std::optional<SceneTaskReadView> task;
};
struct SceneRegistrationRuntime {
    std::optional<SceneTaskReadView> camera, first_player, second_player;
    std::optional<std::int16_t> registered_count; // source3b2d510
    std::vector<SceneRegisteredLandEntry> registered_entries;
    std::optional<float> view_distance; // source915458
};
struct SceneRegistrationConstants { float second_view_distance; }; // source7e0028, exact bits42200000
// Actual43aef0 registry/culling with source fixed128 dynamic/static limits,
// live camera/player readviews and real decoded landtable records. Rendering
// and texture loading in43a260 are separate; no missing view becomes an origin.
SceneQueryResult register_player_scene_landscape(PlayerSceneLandscape&,
    const DecodedSceneLandTable&,const SceneRegistrationRuntime&,const SceneRegistrationConstants&,
    PlayerCoordinates&,X87Context&);
} // namespace sadx
