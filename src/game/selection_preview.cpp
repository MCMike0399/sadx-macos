#include "game/selection_preview.hpp"
#include "cpu/fidelity_float.hpp"

namespace sadx::selection_preview {
namespace {
// NOTSA: reads the source task's current auxiliary owner, not a cached work object.
std::shared_ptr<Work> work_of(const TaskHandle &task) {
    if (!task || !task->auxiliary_native_work)
        throw Error(Boundary::Work, 0, "character preview task has no work");
    return std::static_pointer_cast<Work>(task->auxiliary_native_work);
}
// NOTSA: source dereferences every actor after the creation loop, including failed slots.
Actor actor_at(Work &work, std::size_t slot, Calls &calls) {
    if (!work.actors[slot])
        throw Error(Boundary::Actor, slot, "character preview actor is absent");
    return calls.actor(work.actors[slot]);
}
} // namespace

void reset_actors(Work &work, Assets assets, Calls &calls) {
    for (std::size_t slot = 0; slot < work.actors.size(); ++slot) {
        auto actor = actor_at(work, slot, calls);
        actor.pose.position.x = 400.0f;
        actor.pose.position.y = 0;
        actor.pose.position.z = -20.0f;
        actor.pose.rotation[0] = 0;
        actor.pose.rotation[2] = 0;
        if (slot != 6)
            actor.pose.rotation[1] = 16384;
        if (slot == 0)
            actor.aim_y = assets.sonic_aim_y;
    }
}

void prepare_actors(Work &work, State &state, Assets assets, View view, Calls &calls,
                    X87Context &fp) {
    for (std::size_t slot = 0; slot < work.actors.size(); ++slot) {
        work.actors[slot].reset();
        work.motion_counts[slot] = 0;
        const auto &position = assets.positions[slot];
        const auto z =
            (fidelity::ExactFloat(fp, view.horizontal_scale) * position.z).rounded();
        state.positions[slot].z = z;
        state.positions[slot].x = position.x;
        state.positions[slot].y = position.y;
        if (slot != 6)
            calls.load_character_textures(assets.characters[slot]);
        const auto actor = calls.create_actor(static_cast<ActorKind>(slot), 7, 1);
        work.actors[slot] = actor;
        if (!actor)
            continue;
        auto fields = calls.actor(actor);
        fields.pose.rotation[1] = 16384;
        if (slot == 5)
            fields.task_id = 2;
        fields.character_id = static_cast<std::uint8_t>(assets.characters[slot]);
        fields.player_index = static_cast<std::uint8_t>(slot);
    }
    reset_actors(work, assets, calls);
    calls.set_perspective(12743);
    view.saved_view_angle = 12743;
}

void initialize(const TaskHandle &task, State &state, Assets assets, View view, Calls &calls,
                X87Context &fp) {
    if (!task)
        throw Error(Boundary::Task, 0, "character preview initialization has no task");
    const auto owner = std::static_pointer_cast<Work>(task->auxiliary_native_work);
    calls.select_palette(2);
    const auto camera = calls.create_camera(6, 0);
    if (!owner)
        throw Error(Boundary::Work, 0, "character preview allocation returned null");
    auto &work = *owner;
    work.camera = camera;
    if (camera) {
        auto pose = calls.camera_pose(camera);
        pose.rotation[2] = 0;
        pose.rotation[1] = 0;
        pose.rotation[0] = 0;
        pose.position.z = 0;
        pose.position.y = 0;
        pose.position.x = 0;
    }
    prepare_actors(work, state, assets, view, calls, fp);
    work.warmup_frames = 0;
    work.selected_motion = 0;
}

void create(State &state, Assets assets, View view, Calls &calls, X87Context &fp) {
    const auto task = calls.create_preview_task(0, 3);
    state.task = task;
    if (!task)
        return;
    const auto work = calls.allocate_work(task);
    task->auxiliary_native_work = work;
    initialize(task, state, assets, view, calls, fp);
    calls.install_callbacks(task);
    state.reset_marker = 0;
}

void position_selected(Work &work, State &state, Assets assets, View view, Calls &calls) {
    reset_actors(work, assets, calls);
    const auto selected = view.selected_character;
    if (selected >= work.actors.size())
        throw Error(Boundary::ActorIndex, selected, "selected preview actor is out of range");
    auto actor = actor_at(work, selected, calls);
    actor.pose.position.x = state.positions[selected].x;
    actor.pose.position.y = state.positions[selected].y;
    actor.pose.position.z = state.positions[selected].z;
}

void update(const TaskHandle &task, State &state, Assets assets, View view, Calls &calls) {
    const auto owner = work_of(task);
    auto &work = *owner;
    if (work.warmup_frames < 2) {
        reset_actors(work, assets, calls);
        work.warmup_frames = static_cast<std::int8_t>(
            static_cast<std::uint8_t>(work.warmup_frames) + 1U);
        return;
    }
    position_selected(work, state, assets, view, calls);
}

void draw(const TaskHandle &task, View view, Calls &calls) {
    if (!task)
        throw Error(Boundary::Task, 0, "character preview draw has no task");
    const auto owner = std::static_pointer_cast<Work>(task->auxiliary_native_work);
    calls.set_perspective(12743);
    view.saved_view_angle = 12743;
    if (!owner)
        throw Error(Boundary::Work, 0, "character preview draw has no work");
    if (owner->warmup_frames >= 2)
        calls.update_actor_actions(*owner);
}

void release_actors(Work &work, Assets assets, Calls &calls) {
    for (std::size_t slot = 0; slot < work.actors.size(); ++slot) {
        const auto actor = work.actors[slot];
        if (actor)
            calls.destroy_task(actor);
        if (slot != 6)
            calls.release_actor_textures(assets.characters[slot]);
    }
}

void destroy(const TaskHandle &task, Assets assets, Calls &calls) {
    const auto owner = work_of(task);
    auto &work = *owner;
    const auto camera = work.camera;
    if (camera)
        calls.destroy_task(camera);
    release_actors(work, assets, calls);
}

void remove(State &state, Calls &calls) {
    const auto task = state.task;
    if (task)
        calls.destroy_task(task);
    state.task.reset();
}
} // namespace sadx::selection_preview
