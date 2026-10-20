#include "player/player_direction_reset.hpp"
#include "player/player_scene_position_internal.hpp"
#include <algorithm>

namespace sadx::scene {
namespace {
// NOTSA: fail explicitly while a recovered callee still exposes a legacy interface.
[[noreturn]] void uncomposed(const char *callee) { throw Error(Failure::UncomposedCallee, callee); }
// NOTSA: plain borrowed native owners for the same dispatcher used by the reference build.
struct NativeDispatcher final : Context {
    NativeDispatcher(Runtime &runtime, const Constants &constants)
        : Context(runtime.actor.orientation, runtime.player.motion, runtime.parameters,
                  runtime.actor.player_index, runtime.character_id, runtime.player.alignment,
                  constants, runtime.coordinates, runtime.arithmetic, {0.0f, 1.0f}),
          runtime(runtime) {}
    Runtime &runtime;
    Contact backup{};
    RegisteredPlayer selected{};
    std::array<LandEntry, 64> first{}, ordered{};
    std::array<bool, 64> first_written{}, ordered_written{};
    NinjaVector &position(Access) override { return runtime.actor.position; }
    void position(const NinjaVector &value) override { runtime.actor.position = value; }
    std::uint8_t &action(Access) override { return runtime.action; }
    std::uint16_t &status(Access) override { return runtime.player.status; }
    void status(const std::uint16_t &value) override { runtime.player.status = value; }
    std::uint32_t &prior_attributes(Access) override {
        return runtime.player.prior_attributes;
    }
    NinjaVector &floor_normal(Access) override { return runtime.player.floor_normal; }
    void floor_normal(const NinjaVector &value) override { runtime.player.floor_normal = value; }
    TaskHandle &moving_task(Access) override { return runtime.player.moving_task; }
    void moving_task(const TaskHandle &value) override { runtime.player.moving_task = value; }
    SceneResourceReference &cached_object(Access) override {
        return runtime.player.cached_object;
    }
    void cached_object(const SceneResourceReference &value) override {
        runtime.player.cached_object = value;
    }
    std::int16_t &no_control_time(Access) override { return runtime.player.no_control_time; }
    NinjaVector &collision_normal(Access) override { return runtime.player.collision_normal; }
    void collision_normal(const NinjaVector &value) override {
        runtime.player.collision_normal = value;
    }
    std::uint32_t &special_surface(Access) override { return runtime.globals.special_surface; }
    void special_surface(const std::uint32_t &value) override {
        runtime.globals.special_surface = value;
    }
    std::uint32_t &motion_override(Access) override { return runtime.globals.motion_override; }
    TaskHandle &event_task(Access) override { return runtime.globals.event_task; }
    std::uint8_t &environment(Access) override { return runtime.globals.environment; }
    std::int8_t &render_mode(Access) override { return runtime.globals.render_mode; }
    float &amy_center_offset(Access) override { return runtime.globals.amy_center_offset; }
    float &amy_radius_offset(Access) override { return runtime.globals.amy_radius_offset; }
    const NinjaVector &gravity(Access) override { return runtime.globals.gravity; }
    float &radius(Access) override { return runtime.contact.radius; }
    void radius(const float &value) override { runtime.contact.radius = value; }
    NinjaVector &center(Access) override { return runtime.contact.center; }
    void center(const NinjaVector &value) override { runtime.contact.center = value; }
    NinjaVector &displacement(Access) override { return runtime.contact.displacement; }
    void displacement(const NinjaVector &value) override { runtime.contact.displacement = value; }
    NinjaVector &direction(Access) override { return runtime.contact.direction; }
    void direction(const NinjaVector &value) override { runtime.contact.direction = value; }
    std::int32_t &contact_pitch(Access) override { return runtime.contact.pitch; }
    void contact_pitch(const std::int32_t &value) override { runtime.contact.pitch = value; }
    std::int32_t &contact_roll(Access) override { return runtime.contact.roll; }
    void contact_roll(const std::int32_t &value) override { runtime.contact.roll = value; }
    NinjaVector &response_normal(Access) override { return runtime.contact.response_normal; }
    void response_normal(const NinjaVector &value) override {
        runtime.contact.response_normal = value;
    }
    NinjaVector &surface_normal(Access) override { return runtime.contact.surface_normal; }
    void surface_normal(const NinjaVector &value) override {
        runtime.contact.surface_normal = value;
    }
    NinjaVector &response_plane(Access) override { return runtime.contact.response_plane; }
    void response_plane(const NinjaVector &value) override {
        runtime.contact.response_plane = value;
    }
    NinjaVector &corrected_plane(Access) override { return runtime.contact.corrected_plane; }
    void corrected_plane(const NinjaVector &value) override {
        runtime.contact.corrected_plane = value;
    }
    NinjaVector &slide_vector(Access) override { return runtime.contact.slide_vector; }
    void slide_vector(const NinjaVector &value) override { runtime.contact.slide_vector = value; }
    std::uint16_t &selected_contact(Access) override {
        return runtime.contact.selected_contact;
    }
    std::uint32_t &registered_attributes(Access) override {
        return selected.player->prior_attributes;
    }
    std::uint16_t &registered_status(Access) override { return selected.player->status; }

    void bind_contact(Access) override {}
    void require_world(Access) override {}
    NinjaVector &world_velocity(Access) override { return runtime.world.motion.speed; }
    void world_pitch(std::int32_t value) override { runtime.world.pitch = value; }
    void world_roll(std::int32_t value) override { runtime.world.roll = value; }
    RegisteredPlayer &registered_player() {
        if (player_index >= runtime.registered.size())
            throw Error(Failure::MissingRegisteredPlayer, "registered player index");
        return runtime.registered[player_index];
    }
    WorldMotion &registered_world() {
        auto *world = registered_player().world;
        if (!world)
            throw Error(Failure::MissingRegisteredPlayer, "registered world motion");
        return *world;
    }
    NinjaVector &registered_velocity(Access) override {
        return registered_world().motion.speed;
    }
    std::int32_t &registered_yaw(Access) override { return registered_world().target_yaw; }
    void select_registered_player() override {
        selected = registered_player();
        if (!selected.player || !selected.parameters)
            throw Error(Failure::MissingRegisteredPlayer, "registered physics and parameters");
    }
    const PlayerParameters &registered_parameters() override { return *selected.parameters; }
    NinjaVector &registered_player_speed() override { return selected.player->motion.speed; }
    void save_contact() override { backup = runtime.contact; }
    void restore_contact() override { runtime.contact = backup; }
    void restore_radius() override { runtime.contact.radius = backup.radius; }
    void restore_displacement() override { runtime.contact.displacement = backup.displacement; }
    std::int32_t candidate_count() override { return runtime.landscape.candidate_count; }
    LandEntry &slot(EntrySlot entry) {
        switch (entry.collection) {
        case EntryCollection::Candidates:
            return runtime.landscape.candidates.at(entry.index);
        case EntryCollection::FirstPass:
            if (!first_written.at(entry.index))
                throw Error(Failure::UninitializedEntry, "unwritten first-pass entry");
            return first.at(entry.index);
        case EntryCollection::Ordered:
            if (!ordered_written.at(entry.index))
                throw Error(Failure::UninitializedEntry, "unwritten ordered entry");
            return ordered.at(entry.index);
        }
        throw Error(Failure::UninitializedEntry, "unknown scene-entry collection");
    }
    EntrySlot entry(EntryCollection collection, std::size_t index, Access) override {
        const EntrySlot value{collection, index};
        (void)slot(value);
        return value;
    }
    std::uint32_t entry_attributes(EntrySlot entry, Access) override {
        return slot(entry).attributes;
    }
    SceneResourceReference entry_object(EntrySlot entry, Access) override {
        return slot(entry).object;
    }
    SceneResourceReference entry_task(EntrySlot entry, Access) override {
        return slot(entry).task;
    }
    void copy_entry(EntrySlot source, EntrySlot destination) override {
        const auto value = slot(source);
        switch (destination.collection) {
        case EntryCollection::Candidates:
            runtime.landscape.candidates.at(destination.index) = value;
            break;
        case EntryCollection::FirstPass:
            first.at(destination.index) = value;
            first_written.at(destination.index) = true;
            break;
        case EntryCollection::Ordered:
            ordered.at(destination.index) = value;
            ordered_written.at(destination.index) = true;
            break;
        }
    }
    void swap_ordered(std::size_t a, std::size_t b) override {
        std::swap(ordered.at(a), ordered.at(b));
        std::swap(ordered_written.at(a), ordered_written.at(b));
    }
    GeometryTask &geometry(const SceneResourceReference &ref) {
        if (!ref.id)
            throw Error(Failure::MissingGeometry, "null geometry resource");
        const auto found = runtime.geometry.find(*ref.id);
        if (found == runtime.geometry.end() || !found->second.task)
            throw Error(Failure::MissingGeometry, "geometry resource owner");
        return found->second;
    }
    GeometryTask &geometry(const TaskHandle &task) {
        const auto found =
            std::find_if(runtime.geometry.begin(), runtime.geometry.end(),
                         [&](const auto &entry) { return entry.second.task == task; });
        if (found == runtime.geometry.end())
            throw Error(Failure::MissingGeometry, "geometry task owner");
        return found->second;
    }
    TaskHandle geometry_task(const SceneResourceReference &ref, Access) override {
        return geometry(ref).task;
    }
    bool geometry_has_primary(const TaskHandle &task) override {
        return geometry(task).has_primary;
    }
    std::uint16_t geometry_flags(const TaskHandle &task, Access) override {
        return geometry(task).flags;
    }
    // NOTSA: STUB for native broadphase composition; no invented empty landscape.
    void broadphase(Access) override { uncomposed("query_player_scene_broadphase"); }
    // NOTSA: STUB for native mesh-query composition; no invented no-contact result.
    bool query(EntrySlot, Access) override { uncomposed("query_mesh_object_contacts"); }
    // NOTSA: STUB for native contact-response composition.
    std::int32_t respond(Response, std::uint32_t, Access) override {
        uncomposed("respond_player_contacts");
    }
    // NOTSA: STUB for native attribute/default-parameter composition.
    void attributes(std::uint32_t, Access) override {
        uncomposed("apply_player_contact_attributes");
    }
    // NOTSA: STUB for native CRT policy composition in projection angles.
    std::int32_t angle(const NinjaVector &, const NinjaVector &, const NinjaVector *,
                       Access) override {
        uncomposed("projection_vector_angle");
    }
    // NOTSA: STUB for native contact-initializer composition; preceding stores survive.
    void initialize_offset(NinjaVector &, ActorSelection, Access) override {
        uncomposed("initialize_player_contact_offset");
    }
    void task_to_world(NinjaVector &value, Access) override {
        if (!coordinates.task_to_world(&orientation, value, fp))
            throw Error(Failure::CoordinateFailed, "task_to_world");
    }
    void calculate_vector(const NinjaMatrix &matrix, const NinjaVector &value, NinjaVector &out,
                          Access) override {
        if (!coordinates.calculate_vector(matrix, value, out, fp))
            throw Error(Failure::CoordinateFailed, "calculate_vector");
    }
    // NOTSA: STUB for native normal-to-angle composition in floor transforms.
    void floor_to_world(const NinjaVector &, NinjaVector &, Access) override {
        uncomposed("floor_to_world normal-angle owner");
    }
    void intersect_planes(const NinjaPlane &a, const NinjaPlane &b, NinjaPlane &out) override {
        const PlayerPlaneConstants plane{{0.0f, 1.0f}, 0.975f, 1.0};
        (void)intersect_source_planes(a, b, &out, plane, fp);
    }
    void require_selected_response(std::uint16_t selected, Access) override {
        if (selected >= runtime.response_points.size())
            throw Error(Failure::MissingSelectedResponse, "selected contact point");
    }
    void project_to_response_plane(const NinjaVector &point, std::uint16_t selected,
                                   const NinjaVector &normal, NinjaVector &out) override {
        (void)project_point_onto_plane(point, runtime.response_points.at(selected), normal, &out,
                                       1.0, fp);
    }
    // NOTSA: STUB for native floor-angle CRT owner composition.
    void store_floor_angles(const NinjaVector &, Access) override {
        uncomposed("floor_normal_angles");
    }
    // NOTSA: STUB for the native source CRT policy owner.
    fidelity::ExactFloat asin(float, Access) override { uncomposed("crt_asin"); }
    void rotate_floor(Access) override {
        const FloorContact contact{runtime.contact.direction, runtime.contact.surface_normal,
                                   runtime.contact.pitch, runtime.contact.roll};
        scene::rotate_scene_floor(runtime.actor, runtime.player.motion, runtime.player.status,
                                  runtime.parameters, contact, runtime.globals.gravity, coordinates,
                                  fp);
    }
    void nudge_normal(Access) override {
        scene::nudge_scene_normal(runtime.actor, runtime.world.motion,
                                  runtime.contact.response_normal, fp);
    }
    void move_platform(const NinjaVector &offset, Access) override {
        const auto task = runtime.player.moving_task;
        if (!task)
            throw Error(Failure::NullMovingTask, "moving platform task");
        PlatformWork *work = nullptr;
        if (task->force_work) {
            const auto found = runtime.platforms.find(task.get());
            if (found == runtime.platforms.end() || !found->second)
                throw Error(Failure::MissingPlatformWork, "moving platform force work");
            work = found->second;
        }
        scene::move_scene_platform(runtime.actor, runtime.player.motion, runtime.contact.center,
                                   offset, work, runtime.tasks, fp);
    }
    // NOTSA: STUB for native input/camera owner composition.
    Input read_input(Access) override { uncomposed("get_player_input"); }
    // NOTSA: STUB for the unrecovered contact marker's material/model rendering.
    void draw_contact_marker() override { uncomposed("draw_contact_marker"); }
    [[noreturn]] void fail(Failure failure, Access) override {
        throw Error(failure, "scene positioning");
    }
};
} // namespace
// NOTSA: native composition, independent of guest archives and executable data.
std::int32_t position_player_in_scene(Runtime &runtime) {
    const Constants constants;
    NativeDispatcher context(runtime, constants);
    return position_player_in_scene(context);
}
// NOTSA: alternate native composition over the same scene owners.
std::int32_t position_player_in_scene_alternate(Runtime &runtime) {
    const Constants constants;
    NativeDispatcher context(runtime, constants);
    return position_player_in_scene_alternate(context);
}
} // namespace sadx::scene
