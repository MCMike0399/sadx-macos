#include "runtime/native_player_system.hpp"

namespace sadx::native {
void bind_gamma_resources(PlayerSystem &players, character_pack::SceneState &scene,
                          const GammaActorAssets &assets) {
    // These are declarations of the one source table, not per-player copies.
    PlayerJoinTable weld;
    for (const auto &row : assets.joins) {
        PlayerJoinVertex vertex{};
        if (row.object) vertex.object = &scene.objects.object(row.object);
        if (row.source_object)
            vertex.source_object = &scene.objects.object(row.source_object);
        if (row.destination_object)
            vertex.destination_object = &scene.objects.object(row.destination_object);
        vertex.vertex_count = row.vertex_count;
        vertex.input_mode = row.input_mode;
        vertex.source_depth = row.source_depth;
        vertex.destination_depth = row.destination_depth;
        weld.rows.push_back(vertex);
    }
    gamma_cursors::Templates templates;
    const std::array<gamma_cursors::WireModel *, 2> destinations = {&templates.first, &templates.second};
    for (std::size_t index = 0; index < assets.cursors.size(); ++index) {
        const auto &source = assets.cursors[index];
        auto &cursor = *destinations[index];
        cursor.points = {&scene.geometry.vector_words(source.points), 0};
        cursor.colors = {&scene.geometry.color_words(source.colors), 0};
        cursor.indices = &scene.geometry.indices(source.indices);
        cursor.line_count = source.line_count;
        cursor.padding = source.padding;
    }
    players.gamma_weld = std::move(weld);
    players.gamma_cursor_templates = std::move(templates);
}
} // namespace sadx::native
