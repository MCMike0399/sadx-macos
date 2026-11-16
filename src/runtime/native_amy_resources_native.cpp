#include "runtime/native_player_system.hpp"

namespace sadx::native {
void bind_amy_resources(PlayerSystem &players, character_pack::SceneState &scene,
                        const AmyActorAssets &assets) {
    PlayerJoinTable weld;
    std::size_t join_index = 0;
    for (const auto &row : assets.joins) {
        PlayerJoinVertex vertex{};
        if (row.object)
            vertex.object = &scene.objects.object(row.object);
        if (row.source_object)
            vertex.source_object = &scene.objects.object(row.source_object);
        if (row.destination_object)
            vertex.destination_object = &scene.objects.object(row.destination_object);
        vertex.vertex_count = row.vertex_count;
        vertex.input_mode = row.input_mode;
        vertex.source_depth = row.source_depth;
        vertex.destination_depth = row.destination_depth;
        if (row.vertex_pairs)
            vertex.vertex_pairs = &assets.join_pairs[join_index];
        ++join_index;
        weld.rows.push_back(vertex);
    }
    players.amy_weld = std::move(weld);
}
} // namespace sadx::native
