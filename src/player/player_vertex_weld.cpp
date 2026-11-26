#include "player/player_vertex_weld.hpp"
#include "engine/ninja_matrix_composition.hpp"
#include "player/model_motion_source.hpp"
#include "player/animation/animation_native_access.hpp"
#include "player/player_mesh_matrix.hpp"
#include <cstring>

namespace sadx::vertex_weld {
namespace {
using fidelity::ExactFloat;
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
// NOTSA: write one source MOV pair into the mutable imported vector array.
void write_components(std::vector<std::uint32_t> &words, std::uint16_t index,
                      const NinjaVector &value, std::size_t row, unsigned vertex) {
    const std::array<float, kVectorComponents> components{value.x, value.y, value.z};
    for (unsigned component = 0; component < kVectorComponents; ++component) {
        const auto destination = static_cast<std::size_t>(index) * kVectorComponents + component;
        if (destination >= words.size())
            throw Error(Boundary::VectorWrite, row, vertex, component);
        std::memcpy(&words[destination], &components[component], sizeof(float));
    }
}
void write_geometry(character_pack::GeometryState &geometry,
                    character_pack::ResourceId resource, std::uint16_t index,
                    const NinjaVector &value, std::size_t row, unsigned vertex) {
    auto *words = resource ? &geometry.vector_words(resource) : nullptr;
    if (!words)
        throw Error(Boundary::VectorWrite, row, vertex, 0);
    write_components(*words, index, value, row, vertex);
}
NinjaVector read_backup(TaskWorkArena &arena, const PlayerJoinVertex &join, unsigned vertex,
                        unsigned vector, std::size_t row) {
    std::array<std::uint32_t, kVectorComponents> words{};
    const auto result = arena.read(join.original_vertices,
                                   vertex * kBackupBytes + vector * kVectorBytes, words.data(),
                                   kVectorBytes);
    if (!result)
        throw Error(Boundary::BackupRead, row, vertex, vector, result.failure);
    std::array<float, kVectorComponents> components{};
    for (unsigned component = 0; component < kVectorComponents; ++component)
        std::memcpy(&components[component], &words[component], sizeof(float));
    return {components[0], components[1], components[2]};
}
// NOTSA: the source's point average is destination first, then source, then *0.5.
NinjaVector average(const NinjaVector &destination, const NinjaVector &source,
                    fidelity::Context &fp) {
    const auto x = (ExactFloat(fp, destination.x) + source.x) * 0.5f;
    const auto y = (ExactFloat(fp, destination.y) + source.y) * 0.5f;
    const auto z = (ExactFloat(fp, destination.z) + source.z) * 0.5f;
    return {x.rounded(), y.rounded(), z.rounded()};
}
// NOTSA: the source sums destination first, then source; the caller normalizes.
NinjaVector sum(const NinjaVector &destination, const NinjaVector &source,
                fidelity::Context &fp) {
    const auto x = ExactFloat(fp, destination.x) + source.x;
    const auto y = ExactFloat(fp, destination.y) + source.y;
    const auto z = ExactFloat(fp, destination.z) + source.z;
    return {x.rounded(), y.rounded(), z.rounded()};
}
// NOTSA: one of the six consecutive playerwk attachment pairs selected by mode.
NinjaVector &attachment_point(PlayerAttachmentVectors &attachments, unsigned mode) {
    switch (mode) {
    case 4: return attachments.right_hand_position;
    case 5: return attachments.left_hand_position;
    case 6: return attachments.right_foot_position;
    case 7: return attachments.left_foot_position;
    case 8: return attachments.user0_position;
    case 9: return attachments.user1_position;
    }
    throw Error(Boundary::TableRow, 0, 0, 0);
}
NinjaVector &attachment_direction(PlayerAttachmentVectors &attachments, unsigned mode) {
    switch (mode) {
    case 4: return attachments.right_hand_direction;
    case 5: return attachments.left_hand_direction;
    case 6: return attachments.right_foot_direction;
    case 7: return attachments.left_foot_direction;
    case 8: return attachments.user0_direction;
    case 9: return attachments.user1_direction;
    }
    throw Error(Boundary::TableRow, 0, 0, 0);
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

void process_active(PlayerJoinTable &table, TaskWorkArena &arena, character_pack::SceneState &scene,
                    PlayerAttachmentVectors &attachments, Matrices &matrices, float frame,
                    std::uint32_t node_count, MotionMatrices &motion,
                    PrimitiveOperations &operations, fidelity::Context &fp) {
    motion.calculate(frame, node_count);
    const auto *action_object = motion.action_object();
    for (std::size_t index = 0;; ++index) {
        auto &join = row_at(table, index);
        if (!join.object)
            return;
        if (join.object != action_object || join.input_mode == 0)
            continue;
        if (static_cast<std::int8_t>(join.source_depth) == -1) {
            const auto depth = motion.node_index(join.object, join.source_object);
            join.source_depth = static_cast<std::uint8_t>(depth);
            if (static_cast<std::int8_t>(join.source_depth) == -1)
                continue;
        }
        (void)motion.get(join.source_depth, matrices.source);
        if (!join.destination_object) {
            const auto mode = static_cast<unsigned>(join.input_mode);
            NinjaVector point{};
            NinjaVector direction{};
            if (join.vertex_count == 0)
                direction.x = 1.0f;
            else if (join.vertex_count == 1)
                direction.y = 1.0f;
            else
                direction.z = 1.0f;
            (void)operations.push_matrix(matrices.source);
            if (mode >= 4 && mode <= 9) {
                operations.calculate_point(operations.current_matrix(), point,
                                           attachment_point(attachments, mode));
                operations.calculate_vector(operations.current_matrix(), direction,
                                            attachment_direction(attachments, mode));
            }
            operations.pop_matrix();
            continue;
        }
        if (static_cast<std::int8_t>(join.destination_depth) == -1) {
            const auto depth = motion.node_index(join.object, join.destination_object);
            join.destination_depth = static_cast<std::uint8_t>(depth);
            if (static_cast<std::int8_t>(join.destination_depth) == -1)
                continue;
        }
        operations.set_matrix(matrices.source_inverse, matrices.source);
        operations.invert_matrix(matrices.source_inverse);
        (void)motion.get(join.destination_depth, matrices.destination);
        operations.set_matrix(matrices.destination_inverse, matrices.destination);
        operations.invert_matrix(matrices.destination_inverse);
        const auto mode = static_cast<unsigned>(join.input_mode);
        const auto source = model_resource(join.source_object, index);
        const auto destination = model_resource(join.destination_object, index);
        const auto source_points = model_vector(source, false, scene.geometry, index, 0);
        const auto source_normals = model_vector(source, true, scene.geometry, index, 0);
        const auto destination_points = model_vector(destination, false, scene.geometry, index, 0);
        const auto destination_normals = model_vector(destination, true, scene.geometry, index, 0);
        if (mode == 1) {
            for (unsigned vertex = 0; vertex < join.vertex_count; ++vertex) {
                const auto source_point_index = pair(join, vertex, 0, index);
                const auto destination_point_index = pair(join, vertex, 1, index);
                const auto source_backup_point = read_backup(arena, join, vertex, 0, index);
                const auto destination_backup_point = read_backup(arena, join, vertex, 2, index);
                NinjaVector source_point, destination_point;
                operations.calculate_point(matrices.source, source_backup_point, source_point);
                operations.calculate_point(matrices.destination, destination_backup_point,
                                           destination_point);
                const auto midpoint = average(destination_point, source_point, fp);
                NinjaVector source_target, destination_target;
                operations.calculate_point(matrices.source_inverse, midpoint, source_target);
                write_geometry(scene.geometry, source_points, source_point_index, source_target,
                               index, vertex);
                operations.calculate_point(matrices.destination_inverse, midpoint, destination_target);
                write_geometry(scene.geometry, destination_points, destination_point_index,
                               destination_target, index, vertex);
                const auto source_backup_normal = read_backup(arena, join, vertex, 1, index);
                const auto destination_backup_normal = read_backup(arena, join, vertex, 3, index);
                NinjaVector source_normal, destination_normal;
                operations.calculate_vector(matrices.source, source_backup_normal, source_normal);
                operations.calculate_vector(matrices.destination, destination_backup_normal,
                                            destination_normal);
                auto normal = sum(destination_normal, source_normal, fp);
                operations.unit_vector(normal);
                NinjaVector source_normal_target, destination_normal_target;
                operations.calculate_vector(matrices.source_inverse, normal, source_normal_target);
                write_geometry(scene.geometry, source_normals, source_point_index,
                               source_normal_target, index, vertex);
                operations.calculate_vector(matrices.destination_inverse, normal,
                                            destination_normal_target);
                write_geometry(scene.geometry, destination_normals, destination_point_index,
                               destination_normal_target, index, vertex);
            }
        } else if (mode == 2) {
            (void)operations.push_matrix(matrices.destination_inverse);
            operations.multiply_current(matrices.source);
            for (unsigned vertex = 0; vertex < join.vertex_count; ++vertex) {
                const auto source_index = pair(join, vertex, 0, index);
                const auto destination_index = pair(join, vertex, 1, index);
                const auto source_backup_point = read_backup(arena, join, vertex, 0, index);
                write_geometry(scene.geometry, source_points, source_index, source_backup_point,
                               index, vertex);
                NinjaVector destination_point;
                operations.calculate_point(operations.current_matrix(), source_backup_point,
                                           destination_point);
                write_geometry(scene.geometry, destination_points, destination_index,
                               destination_point, index, vertex);
                const auto source_backup_normal = read_backup(arena, join, vertex, 1, index);
                write_geometry(scene.geometry, source_normals, source_index, source_backup_normal,
                               index, vertex);
                NinjaVector destination_normal;
                operations.calculate_vector(operations.current_matrix(), source_backup_normal,
                                            destination_normal);
                write_geometry(scene.geometry, destination_normals, destination_index,
                               destination_normal, index, vertex);
            }
            operations.pop_matrix();
        } else if (mode == 3) {
            (void)operations.push_matrix(matrices.source_inverse);
            operations.multiply_current(matrices.destination);
            for (unsigned vertex = 0; vertex < join.vertex_count; ++vertex) {
                const auto source_index = pair(join, vertex, 0, index);
                const auto destination_index = pair(join, vertex, 1, index);
                const auto destination_backup_point = read_backup(arena, join, vertex, 2, index);
                write_geometry(scene.geometry, destination_points, destination_index,
                               destination_backup_point, index, vertex);
                NinjaVector source_point;
                operations.calculate_point(operations.current_matrix(), destination_backup_point,
                                           source_point);
                write_geometry(scene.geometry, source_points, source_index, source_point, index,
                               vertex);
                const auto destination_backup_normal = read_backup(arena, join, vertex, 3, index);
                write_geometry(scene.geometry, destination_normals, destination_index,
                               destination_backup_normal, index, vertex);
                NinjaVector source_normal;
                operations.calculate_vector(operations.current_matrix(), destination_backup_normal,
                                            source_normal);
                write_geometry(scene.geometry, source_normals, source_index, source_normal, index,
                               vertex);
            }
            operations.pop_matrix();
        }
    }
}

std::unique_ptr<model_matrices::MotionSource>
active_motion_source(PlayerPhysicsState &physics, TaskWorkArena &arena,
                     const animation::Resources &resources,
                     const character_pack::ObjectState &objects, float &frame,
                     std::uint32_t &node_count) {
    try {
        animation::View view{physics, {}};
        auto fields = animation::detail::NativeAccess{}.state(view);
        const auto &table = animation::detail::NativeAccess{}.table(view, animation::Phase::None);
        if (!fields.requested || !fields.mode || !fields.frame)
            return nullptr;
        const auto found = table.records.find(*fields.requested);
        if (found == table.records.end())
            return nullptr;
        const auto &record = found->second;
        frame = *fields.frame;
        node_count = record.object_count;
        if (*fields.mode == 2) {
            if (!fields.buffer || !*fields.buffer)
                return nullptr;
            return std::make_unique<model_matrices::NativeMotionSource>(**fields.buffer, arena,
                                                                        objects);
        }
        if (record.action.status != animation::ResourceStatus::Loaded)
            return nullptr;
        const auto action = resources.actions.find(record.action.identity);
        if (action == resources.actions.end())
            return nullptr;
        return std::make_unique<model_matrices::NativeMotionSource>(action->second, resources,
                                                                    objects);
    } catch (const animation::Stop &) {
        return nullptr;
    }
}

namespace {
class NativeOperations final : public PrimitiveOperations {
  public:
    NativeOperations(NinjaMatrixPrimitives &coordinates, fidelity::Context &fp)
        : coordinates_(coordinates), fp_(fp) {}
    void set_matrix(NinjaMatrix &destination, const NinjaMatrix &source) override {
        destination = source;
    }
    void invert_matrix(NinjaMatrix &matrix) override {
        // The source ignores the boolean; a singular matrix keeps its sentinel.
        (void)invert_mesh_matrix(matrix, {0.0f, 1.0f}, fp_);
    }
    void multiply_current(const NinjaMatrix &source) override {
        multiply_ninja_matrix(coordinates_.current_matrix(), source, fp_);
    }
    bool push_matrix(const NinjaMatrix &matrix) override {
        return coordinates_.push_supplied_frame(matrix);
    }
    void pop_matrix() override { coordinates_.end_ex_frame(); }
    NinjaMatrix &current_matrix() override { return coordinates_.current_matrix(); }
    void calculate_point(const NinjaMatrix &matrix, const NinjaVector &input,
                         NinjaVector &output) override {
        coordinates_.calculate_point(matrix, input, output, fp_);
    }
    void calculate_vector(const NinjaMatrix &matrix, const NinjaVector &input,
                          NinjaVector &output) override {
        const auto result = coordinates_.calculate_vector(matrix, input, output, fp_);
        if (!result)
            throw Error(Boundary::MatrixTransform, 0, 0, 0);
    }
    void unit_vector(NinjaVector &vector) override {
        (void)normalize_vector_extended(vector, {0.0f, 1.0f}, fp_);
    }

  private:
    NinjaMatrixPrimitives &coordinates_;
    fidelity::Context &fp_;
};

class NativeMotionMatrices final : public MotionMatrices {
  public:
    NativeMotionMatrices(model_matrices::State &state, NinjaMatrixPrimitives &coordinates,
                         model_matrices::MotionSource &motion,
                         const character_pack::ObjectState &objects, fidelity::Context &fp)
        : state_(state), coordinates_(coordinates), motion_(motion), objects_(objects), fp_(fp) {}
    void calculate(float frame, std::uint32_t node_count) override {
        (void)model_matrices::calculate(state_, coordinates_, motion_,
                                        &coordinates_.unit_matrix(), frame, node_count, nullptr,
                                        fp_);
    }
    bool get(std::uint32_t index, NinjaMatrix &destination) override {
        return model_matrices::get(state_, index, destination);
    }
    int node_index(const character_pack::ObjectResource *root,
                   const character_pack::ObjectResource *target) override {
        return vertex_weld::node_index(root, target, objects_);
    }
    const character_pack::ObjectResource *action_object() override {
        const auto id = motion_.root();
        return id ? &motion_.object(id) : nullptr;
    }

  private:
    model_matrices::State &state_;
    NinjaMatrixPrimitives &coordinates_;
    model_matrices::MotionSource &motion_;
    const character_pack::ObjectState &objects_;
    fidelity::Context &fp_;
};
} // namespace

bool run_active(PlayerJoinTable &table, TaskWorkArena &arena,
                character_pack::SceneState &scene, PlayerPhysicsState &physics,
                model_matrices::State &instance_matrices, NinjaMatrixPrimitives &coordinates,
                const animation::Resources &resources, Matrices &matrices,
                fidelity::Context &fp) {
    float frame = 0;
    std::uint32_t node_count = 0;
    auto motion = active_motion_source(physics, arena, resources, scene.objects, frame, node_count);
    if (!motion)
        return false;
    auto motion_matrices =
        native_motion_matrices(instance_matrices, coordinates, *motion, scene.objects, fp);
    auto operations = native_operations(coordinates, fp);
    process_active(table, arena, scene, physics.attachment_vectors, matrices, frame, node_count,
                   *motion_matrices, *operations, fp);
    return true;
}

std::unique_ptr<PrimitiveOperations> native_operations(NinjaMatrixPrimitives &coordinates,
                                                       fidelity::Context &fp) {
    return std::make_unique<NativeOperations>(coordinates, fp);
}

std::unique_ptr<MotionMatrices> native_motion_matrices(model_matrices::State &state,
    NinjaMatrixPrimitives &coordinates, model_matrices::MotionSource &motion,
    const character_pack::ObjectState &objects, fidelity::Context &fp) {
    return std::make_unique<NativeMotionMatrices>(state, coordinates, motion, objects, fp);
}
} // namespace sadx::vertex_weld
