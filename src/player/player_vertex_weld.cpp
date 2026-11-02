#include "player/player_vertex_weld.hpp"

namespace sadx::vertex_weld {
namespace {
constexpr unsigned kVectorComponents = 3;
constexpr unsigned kVectorBytes = 12;
constexpr unsigned kBackupBytes = 48;

// NOTSA: a missing sentinel is a failed source table read, never an empty table.
PlayerJoinVertex &row_at(PlayerJoinTable &table, std::size_t index) {
    if (index >= table.rows.size())
        throw Error(Boundary::TableRow, index, 0, 0);
    return table.rows[index];
}
// NOTSA: capture the model pointer without consuming it. A null model is not a
// fault until a later field read; zero-count rows never perform that read.
character_pack::ResourceId model_resource(const character_pack::ObjectResource *object,
                                          std::size_t row) {
    if (!object)
        throw Error(Boundary::ObjectModel, row, 0, 0);
    return object->model;
}
// NOTSA: consume the captured immutable descriptor at its source field read.
// The vector pointer remains nullable until the actual component access.
character_pack::ResourceId model_vector(character_pack::ResourceId model, bool normal,
                                        const character_pack::GeometryState &geometry,
                                        std::size_t row, unsigned vertex) {
    if (!model)
        throw Error(Boundary::ObjectModel, row, vertex, 0);
    const auto descriptor = geometry.model(model);
    return normal ? descriptor.normals : descriptor.points;
}
// NOTSA: the source reloads each unsigned pair index at its consumption boundary.
std::uint16_t pair(const PlayerJoinVertex &join, unsigned vertex, unsigned side, std::size_t row) {
    const auto offset = vertex * 2 + side;
    if (!join.vertex_pairs || offset >= join.vertex_pairs->size())
        throw Error(Boundary::VertexPair, row, vertex, side);
    return (*join.vertex_pairs)[offset];
}
// NOTSA: preserve three independent MOV reads/stores and their failure prefix.
void save_vector(PlayerJoinVertex &join, TaskWorkArena &arena,
                 character_pack::GeometryState &geometry, character_pack::ResourceId resource,
                 std::uint16_t index, std::size_t row, unsigned vertex, unsigned vector) {
    const auto *words = resource ? &geometry.vector_words(resource) : nullptr;
    TaskWorkHandle allocation;
    for (unsigned component = 0; component < kVectorComponents; ++component) {
        const auto source = static_cast<std::size_t>(index) * kVectorComponents + component;
        if (!words || source >= words->size())
            throw Error(Boundary::VectorRead, row, vertex, component);
        const auto word = (*words)[source];
        if (component == 0)
            allocation = join.original_vertices;
        const auto result = arena.write(
            allocation, vertex * kBackupBytes + vector * kVectorBytes + component * sizeof(word),
            &word, sizeof(word));
        if (!result)
            throw Error(Boundary::BackupWrite, row, vertex, component, result.failure);
    }
}
// NOTSA: restore from the source arena, with no cached backup outside the block.
void restore_vector(PlayerJoinVertex &join, TaskWorkArena &arena,
                    character_pack::GeometryState &geometry, character_pack::ResourceId resource,
                    std::uint16_t index, std::size_t row, unsigned vertex, unsigned vector) {
    auto *words = resource ? &geometry.vector_words(resource) : nullptr;
    const auto allocation = join.original_vertices;
    for (unsigned component = 0; component < kVectorComponents; ++component) {
        std::uint32_t word;
        const auto result = arena.read(
            allocation, vertex * kBackupBytes + vector * kVectorBytes + component * sizeof(word),
            &word, sizeof(word));
        if (!result)
            throw Error(Boundary::BackupRead, row, vertex, component, result.failure);
        const auto destination = static_cast<std::size_t>(index) * kVectorComponents + component;
        if (!words || destination >= words->size())
            throw Error(Boundary::VectorWrite, row, vertex, component);
        (*words)[destination] = word;
    }
}
} // namespace

namespace {
// FUNCTION: SONIC 0x439360 FindPlayerObjectNode
const character_pack::ObjectResource *find_node(const character_pack::ObjectResource *root,
                                                const character_pack::ObjectResource *target,
                                                std::uint32_t &index,
                                                const character_pack::ObjectState &objects) {
    if (root == target)
        return target;
    if (!root)
        throw Error(Boundary::NodeRead, 0, 0, 0);
    ++index;
    if (root->child && find_node(&objects.object(root->child), target, index, objects) == target)
        return target;
    if (root->sibling &&
        find_node(&objects.object(root->sibling), target, index, objects) == target)
        return target;
    return nullptr;
}
} // namespace

int node_index(const character_pack::ObjectResource *root,
               const character_pack::ObjectResource *target,
               const character_pack::ObjectState &objects) {
    if (root == target)
        return target ? 0 : -1;
    if (!root)
        throw Error(Boundary::NodeRead, 0, 0, 0);
    std::uint32_t index = 1;
    if (root->child && find_node(&objects.object(root->child), target, index, objects) == target)
        return target ? static_cast<std::int32_t>(index) : -1;
    if (!root->sibling ||
        find_node(&objects.object(root->sibling), target, index, objects) != target)
        return -1;
    return target ? static_cast<std::int32_t>(index) : -1;
}

void initialize(PlayerJoinTable &table, TaskWorkArena &arena, character_pack::SceneState &scene) {
    for (std::size_t index = 0;; ++index) {
        auto &join = row_at(table, index);
        if (!join.object)
            return;
        if (join.input_mode == 0) {
            // Original quirk: overwritten backups are not released in initialization.
            join.original_vertices = {};
        } else if (join.input_mode >= 1 && join.input_mode <= 3) {
            const auto allocation = arena.allocate(join.vertex_count * kBackupBytes);
            const auto *source_object = join.source_object;
            join.original_vertices = allocation.work;
            // Both objects' model pointers are captured even when the count is zero.
            const auto source = model_resource(source_object, index);
            const auto destination = model_resource(join.destination_object, index);
            for (unsigned vertex = 0; vertex < join.vertex_count; ++vertex) {
                const auto source_points =
                    model_vector(source, false, scene.geometry, index, vertex);
                const auto source_point_index = pair(join, vertex, 0, index);
                save_vector(join, arena, scene.geometry, source_points, source_point_index, index,
                            vertex, 0);
                const auto source_normal_index = pair(join, vertex, 0, index);
                const auto source_normals =
                    model_vector(source, true, scene.geometry, index, vertex);
                save_vector(join, arena, scene.geometry, source_normals, source_normal_index, index,
                            vertex, 1);
                const auto destination_point_index = pair(join, vertex, 1, index);
                const auto destination_points =
                    model_vector(destination, false, scene.geometry, index, vertex);
                save_vector(join, arena, scene.geometry, destination_points,
                            destination_point_index, index, vertex, 2);
                const auto destination_normal_index = pair(join, vertex, 1, index);
                const auto destination_normals =
                    model_vector(destination, true, scene.geometry, index, vertex);
                save_vector(join, arena, scene.geometry, destination_normals,
                            destination_normal_index, index, vertex, 3);
            }
            join.source_depth = static_cast<std::uint8_t>(
                node_index(join.object, join.source_object, scene.objects));
            join.destination_depth = static_cast<std::uint8_t>(
                node_index(join.object, join.destination_object, scene.objects));
        } else if (join.input_mode >= 4 && join.input_mode <= 9) {
            join.original_vertices = {};
            join.source_depth = static_cast<std::uint8_t>(
                node_index(join.object, join.source_object, scene.objects));
            join.destination_depth = 0;
        }
        // Signed byte modes outside 0..9 leave every row field unchanged.
    }
}

void update_prefix(std::int16_t &mode, const std::function<void()> &process_active,
                   const std::function<void()> &initialize_table,
                   const std::function<void()> &restore_table) {
    if (mode == 1) {
        process_active();
    } else if (mode == 0) {
        initialize_table();
        mode = 1;
    } else {
        restore_table();
    }
}

void restore(PlayerJoinTable &table, TaskWorkArena &arena, character_pack::SceneState &scene) {
    for (std::size_t index = 0;; ++index) {
        auto &join = row_at(table, index);
        if (!join.object)
            return;
        if (!join.original_vertices)
            continue;
        const auto source = model_resource(join.source_object, index);
        const auto destination = model_resource(join.destination_object, index);
        for (unsigned vertex = 0; vertex < join.vertex_count; ++vertex) {
            const auto source_points = model_vector(source, false, scene.geometry, index, vertex);
            const auto source_index = pair(join, vertex, 0, index);
            const auto destination_index = pair(join, vertex, 1, index);
            restore_vector(join, arena, scene.geometry, source_points, source_index, index, vertex,
                           0);
            const auto source_normals = model_vector(source, true, scene.geometry, index, vertex);
            restore_vector(join, arena, scene.geometry, source_normals, source_index, index, vertex,
                           1);
            const auto destination_points =
                model_vector(destination, false, scene.geometry, index, vertex);
            restore_vector(join, arena, scene.geometry, destination_points, destination_index,
                           index, vertex, 2);
            const auto destination_normals =
                model_vector(destination, true, scene.geometry, index, vertex);
            restore_vector(join, arena, scene.geometry, destination_normals, destination_index,
                           index, vertex, 3);
        }
        const auto allocation = join.original_vertices;
        const auto result = arena.release(allocation);
        if (!result)
            throw Error(Boundary::ReleaseBackup, index, 0, 0, result.failure);
        join.original_vertices = {};
    }
}
} // namespace sadx::vertex_weld
