#include "apps/native_gamma_actor_assets.hpp"
#include "runtime/gamma_motion.hpp"
#include "runtime/engine_code_tables.hpp"
#include "math/ninja_sine_table.hpp"
#include "player/model_motion_source.hpp"
#include "engine/character_geometry.hpp"
#include <cassert>
#include <fstream>
#include <iostream>

namespace {
using namespace sadx;
void callback(TaskScheduler &, const TaskHandle &) { throw std::runtime_error("unexpected callback"); }
std::uint32_t bits(float value) {
    std::uint32_t result;
    std::memcpy(&result, &value, 4);
    return result;
}
void observe(const char *label, std::ostream &out, const model_matrices::State &state,
             const NinjaMatrixStack &stack, const NinjaMatrix &destination, X87Context &fp) {
    out << label << " state " << state.count << ' ' << state.limit << ' ' << stack.depth()
        << ' ' << fp.control_word() << ' ' << fp.exception_flags() << '\n';
    for (unsigned node = 0; node < 128; ++node) {
        out << label << " matrix " << node;
        for (auto component : state.matrices[node]) out << ' ' << bits(component);
        out << '\n';
    }
    for (unsigned slot = 0; slot < stack.slots().size(); ++slot) {
        out << label << " stack " << slot;
        for (auto component : stack.slots()[slot]) out << ' ' << bits(component);
        out << '\n';
    }
    out << label << " destination 0";
    for (auto component : destination) out << ' ' << bits(component);
    out << '\n';
}
}
int main(int argc, char **argv) {
    using namespace sadx;
    assert(argc == 3);
    const std::filesystem::path directory(argv[1]);
    const auto pack = character_pack::CharacterPack::load(directory / "characters/sonic-selection.actions");
    const auto assets = native::app::load_gamma_actor_assets(directory, pack);
    const native::GammaMotionAssets motions(pack, assets);
    character_pack::SceneState scene(pack);
    const auto &action = motions.resources().actions.at(assets.stand.identity);
    model_matrices::NativeMotionSource imported(action, motions.resources(), scene.objects);
    const auto unit = runtime::engine_code_tables().unit_matrix;
    std::vector<NinjaMatrix> slots(128);
    for (unsigned slot = 0; slot < slots.size(); ++slot)
        for (unsigned component = 0; component < 16; ++component)
            slots[slot][component] = float(slot * 16 + component + 1) / 64;
    const auto original_slots = slots;
    NinjaMatrixStack stack(slots, 1, 0);
    NinjaMatrixPrimitives coordinates(ninja_sine_table(), unit, stack);
    X87Context fp(127);
    model_matrices::State matrices;
    for (unsigned node = 0; node < 128; ++node)
        for (unsigned component = 0; component < 16; ++component)
            matrices.matrices[node][component] = float(node * 16 + component + 17) / 32;
    matrices.count = 19;
    matrices.limit = 23;
    NinjaMatrix destination{};
    for (unsigned component = 0; component < 16; ++component)
        destination[component] = float(component + 39) / 8;
    const auto before = destination;
    assert(!model_matrices::get(matrices, 0xffffffffU, destination) && destination == before);
    std::filesystem::create_directories(std::filesystem::path(argv[2]).parent_path());
    std::ofstream output(argv[2]);
    assert(output);
    fp.clear_exceptions();
    assert(model_matrices::calculate(matrices, coordinates, imported, &unit, 0.0f, 76, &destination, fp));
    assert(matrices.count == 77 && matrices.limit == 77 && stack.depth() == 1);
    assert(stack.current() == original_slots.front());
    assert(destination == matrices.matrices[76] && destination != before);
    assert(model_matrices::get(matrices, 54, destination) && destination == matrices.matrices[54]);
    observe("stand", output, matrices, stack, destination, fp);
    // Real allocation and generated action from the maintained PInitialize/PSetMotion.
    TaskScheduler scheduler;
    auto arena = std::make_shared<TaskWorkArena>();
    assert(scheduler.initialize_source_pool(arena, {}));
    auto task = scheduler.create_element(7, 3, nullptr).task;
    player_initialization::establish_zeroed_primary(scheduler, task);
    player_initialization::establish_zeroed_motion(scheduler, task);
    auto pose = player_initialization::bind_primary_view(task);
    pose.character = 6;
    pose.player_index = 0;
    pose.id = 2;
    PlayerEventFlags events{};
    events[393] = events[465] = 1;
    player_initialization::NativeAccess access(assets.parameters, events, callback);
    player_initialization::Registry registry;
    player_initialization::PlayerHandle player;
    player_initialization::initialize(0, task, player_initialization::bind_existing_motion(task),
                                      scheduler, registry, access, fp, player);
    player_initialization::bind_initialized_player(player).animation_table = assets.animation_table;
    auto &physics = *player->physics;
    animation::View view{physics, player->allocation};
    animation::Runtime runtime{arena.get(), motions.resources(), &scene.objects};
    assert(animation::update(view, runtime, fp));
    auto &tail = physics.animation_tail;
    const auto generated = *tail.buffer_at_148;
    assert(generated && (*generated->motion)->data && generated->allocation);
    auto &live_pose = scene.objects.object(scene.objects.object(*action.object.id).child);
    const auto initial_pose = live_pose.rotation;
    for (unsigned sample = 0; sample < 2; ++sample) {
        assert(animation::release_keys(view, runtime));
        live_pose.rotation = sample == 0 ? std::array<std::int32_t, 3>{1234, -2345, 30000}
                                       : std::array<std::int32_t, 3>{-321, 4321, -12345};
        physics.current_animation_at_126 = 0;
        physics.requested_animation_at_124 = 43;
        physics.animation_mode = 1;
        tail.timer_at_12c = 0;
        tail.frame_at_130 = 4.5f;
        assert(animation::update(view, runtime, fp));
        assert(*tail.buffer_at_148 == generated);
        model_matrices::NativeMotionSource source(*generated, *arena, scene.objects);
        fp.clear_exceptions();
        assert(model_matrices::calculate(matrices, coordinates, source, &unit,
            *tail.frame_at_130, 76, &destination, fp));
        assert(matrices.count == 77 && stack.depth() == 1);
        observe(sample == 0 ? "idle_pose_a" : "idle_pose_b", output, matrices, stack, destination, fp);
    }
    live_pose.rotation = initial_pose;
    // Constructed before the edit: the source must read the live graph again.
    assert(!imported.has_keys(1, model_matrices::Channel::Position));
    assert(model_matrices::calculate(matrices, coordinates, imported, &unit, 0, 76, nullptr, fp));
    const auto original_node = matrices.matrices[1];
    const auto position = live_pose.position;
    live_pose.position[0] += 7;
    assert(model_matrices::calculate(matrices, coordinates, imported, &unit, 0, 76, nullptr, fp));
    assert(matrices.matrices[1] != original_node);
    live_pose.position = position;
    const auto motion = *generated->motion;
    const auto prior_type = motion->type;
    motion->type.reset();
    bool failed = false;
    try {
        model_matrices::NativeMotionSource incomplete(*generated, *arena, scene.objects);
        model_matrices::calculate(matrices, coordinates, incomplete, nullptr, 0, 3, nullptr, fp);
    } catch (const model_matrices::Error &error) {
        failed = error.failure == model_matrices::Failure::MissingMotionType;
    }
    assert(failed && matrices.count == 0 && matrices.limit == 4 && stack.depth() == 2);
    motion->type = prior_type;
    std::cout << "Real Gamma Stand and generated transitions: live scene/key owners, 77 nonzero matrices, retained stack and explicit failure prefix pass\n";
}
