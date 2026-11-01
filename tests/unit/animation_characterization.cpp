// Reference characterization of the existing interface, not a native contract.
// Expected failure prefixes are bounded by reference/animation-44a800.asm,
// animation-blend-43f2a0.asm and ghidra-action-fp-recovery/animation-*.pseudocode.txt.
#include "player/player_animation.hpp"

#include <algorithm>
#include <cstring>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>

using namespace sadx;
namespace {
void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}
std::uint32_t bits(float value) {
    std::uint32_t result;
    std::memcpy(&result, &value, 4);
    return result;
}
struct Fixture {
    PlayerPhysicsState player;
    PlayerAnimationTail tail;
    std::shared_ptr<PlayerAnimationTable> table = std::make_shared<PlayerAnimationTable>();
    PlayerAnimationResources resources;
    std::unordered_map<std::size_t, SceneGeometryObject> objects;
    TaskWorkArena arena;
    TaskWorkHandle work = arena.allocate(512).work;
    PlayerAnimationConstants constants{0, 1, 4294967296.0F};
    PlayerAnimationRuntime runtime{&arena, resources, constants};
    PlayerAnimationView view{player, tail, work};
    X87Context fp{0x37f};
    std::shared_ptr<PlayerAnimationAllocatedAction> action = std::make_shared<PlayerAnimationAllocatedAction>();
    std::shared_ptr<PlayerAnimationAllocatedMotion> motion = std::make_shared<PlayerAnimationAllocatedMotion>();
    Fixture() {
        player.animation_mode_at_120 = 3;
        player.requested_animation_at_124 = 0;
        player.current_animation_at_126 = 0;
        player.previous_animation_at_128 = 77;
        player.motion.speed.x = 2;
        player.source_charge_at_0 = 3;
        tail.next_at_12a = 1;
        tail.timer_at_12c = 65535;
        tail.flags_at_12e = 65535;
        tail.frame_at_130 = 0.5F;
        tail.start_at_134 = 0.25F;
        tail.table_at_140 = table;
        resources.objects = &objects;
        for (unsigned index = 0; index < 2; ++index) {
            table->records[index] = {AnimationResourceReference{index}, 1, 3,
                                     static_cast<std::uint16_t>(index == 0 ? 1 : 0), 0.125F, 0.5F};
            resources.actions[index] = {SceneResourceReference{0}, AnimationResourceReference{index}};
            resources.motions[index] = {10, 3, 2, AnimationResourceReference{index}};
            auto& node = resources.node_arrays[index].emplace_back();
            for (unsigned channel = 0; channel < 3; ++channel) {
                node.keys[channel] = AnimationResourceReference{index * 3 + channel};
                node.counts[channel] = 2;
                auto bytes = std::make_shared<std::vector<std::uint8_t>>(32, 0);
                const std::uint32_t keys[8] = {0, 0x3f800000, 0x40000000, 0x40400000,
                                              4, 0x40a00000, 0x40c00000, 0x40e00000};
                std::memcpy(bytes->data(), keys, 32);
                resources.key_arrays[index * 3 + channel] = {bytes, 0, bytes->size()};
            }
            node.scale_pointer_bits_at_8 = 0;
        }
        auto& object = objects[0];
        object.child = SceneResourceReference{};
        object.sibling = SceneResourceReference{};
        object.position = NinjaVector{11, 13, 17};
        object.angles = std::array<std::int32_t, 3>{19, 23, 29};
        object.scale = NinjaVector{31, 37, 41};
        action->allocation = arena.allocate(8).work;
        motion->allocation = arena.allocate(12).work;
        motion->data = arena.allocate(111 * 24).work;
        require(bool(arena.fill(*motion->data, 0, 0, 111 * 24)), "zero fixture output data");
        motion->frame_count = 2;
        motion->type = 3;
        motion->input_function = 2;
        action->motion = motion;
        action->object = SceneResourceReference{999};
        tail.buffer_at_148 = action;
        require(bool(arena.set_reference_view(work, 0x138, work, 0x38)), "speed alias");
        require(bool(arena.set_reference(work, 0x13c, work)), "charge alias");
    }
    PlayerAnimationResult update() { return update_player_animation(view, runtime, fp); }
    PlayerAnimationResult blend() {
        player.current_animation_at_126 = 1;
        return build_player_animation_transition(view, runtime, fp);
    }
};
unsigned cases = 0;
void record(const std::string& name, Fixture& fixture, const PlayerAnimationResult& result) {
    ++cases;
    std::cout << "{\"case\":\"" << name << "\",\"result\":["
              << static_cast<unsigned>(result.failure) << ',' << result.source_boundary << ','
              << result.callee_address << ',' << result.nested_source_boundary << ','
              << static_cast<unsigned>(result.work_failure.value_or(TaskWorkFailure::none)) << ','
              << result.work_failure.has_value() << ',' << result.completed << ',' << result.wrap_iterations << "],\"state\":["
              << fixture.player.animation_mode_at_120.value_or(-99) << ','
              << fixture.player.requested_animation_at_124.value_or(65535) << ','
              << fixture.player.current_animation_at_126.value_or(65535) << ','
              << fixture.player.previous_animation_at_128.value_or(65535) << ','
              << fixture.tail.flags_at_12e.value_or(65535) << ','
              << fixture.tail.timer_at_12c.value_or(65535) << ','
              << bits(fixture.tail.frame_at_130.value_or(-99)) << ','
              << fixture.fp.exception_flags() << ',' << fixture.fp.control_word() << "],\"known\":["
              << fixture.player.animation_mode_at_120.has_value() << ','
              << fixture.player.requested_animation_at_124.has_value() << ','
              << fixture.player.current_animation_at_126.has_value() << ','
              << fixture.player.previous_animation_at_128.has_value() << ','
              << fixture.tail.flags_at_12e.has_value() << ',' << fixture.tail.timer_at_12c.has_value() << ','
              << fixture.tail.frame_at_130.has_value() << ',' << fixture.tail.start_at_134.has_value() << ','
              << fixture.tail.next_at_12a.has_value() << ',' << fixture.tail.table_at_140.has_value() << ','
              << (fixture.tail.table_at_140 && *fixture.tail.table_at_140 ? 1 : 0) << ','
              << fixture.tail.buffer_at_148.has_value() << ','
              << (fixture.tail.buffer_at_148 && *fixture.tail.buffer_at_148 ? 1 : 0) << "]";
    const auto action = fixture.tail.buffer_at_148.value_or(nullptr);
    const auto motion = action ? action->motion.value_or(nullptr) : nullptr;
    std::cout << ",\"allocated_header\":["
              << (action && action->object.has_value()) << ','
              << (action && action->object && action->object->id.has_value()) << ','
              << (action && action->object && action->object->id ? *action->object->id : 0) << ','
              << (action && action->motion.has_value()) << ',' << bool(motion) << ','
              << (motion && motion->data.has_value()) << ','
              << (motion && motion->data && *motion->data ? 1 : 0) << ','
              << (motion && motion->frame_count.has_value()) << ','
              << (motion ? motion->frame_count.value_or(0) : 0) << ','
              << (motion && motion->type.has_value()) << ',' << (motion ? motion->type.value_or(0) : 0) << ','
              << (motion && motion->input_function.has_value()) << ','
              << (motion ? motion->input_function.value_or(0) : 0) << "]}\n";
}
void failure(const std::string& name, bool blend, const std::function<void(Fixture&)>& change,
             PlayerAnimationFailure expected, std::uint32_t boundary) {
    Fixture fixture;
    change(fixture);
    const auto result = blend ? fixture.blend() : fixture.update();
    record(name, fixture, result);
    if (result.failure != expected || result.source_boundary != boundary) {
        throw std::runtime_error("wrong failure prefix: " + name);
    }
    if (!blend && boundary > 0x44a825) {
        require(fixture.player.previous_animation_at_128 == fixture.player.current_animation_at_126,
                "previous publication precedes later reads");
        require(fixture.tail.flags_at_12e == 0, "flags cleared before later reads");
    }
}
void missingness() {
    using F = PlayerAnimationFailure;
    failure("current", false, [](auto& f) { f.player.current_animation_at_126.reset(); }, F::unknown_field, 0x44a808);
    failure("requested", false, [](auto& f) { f.player.requested_animation_at_124.reset(); }, F::unknown_field, 0x44a80c);
    failure("mode", false, [](auto& f) { f.player.animation_mode_at_120.reset(); }, F::unknown_field, 0x44a825);
    failure("timer", false, [](auto& f) { f.tail.timer_at_12c.reset(); }, F::unknown_field, 0x44a842);
    failure("table-unknown", false, [](auto& f) { f.tail.table_at_140.reset(); }, F::unknown_field, 0x44a948);
    failure("table-null", false, [](auto& f) { f.tail.table_at_140 = std::shared_ptr<const PlayerAnimationTable>{}; }, F::null_resource, 0x44a948);
    failure("row", false, [](auto& f) { f.table->records.erase(0); }, F::unresolved_resource, 0x44a948);
    failure("frame-step", false, [](auto& f) { f.table->records[0].frame_step.reset(); }, F::unknown_field, 0x44a955);
    failure("frame", false, [](auto& f) { f.tail.frame_at_130.reset(); }, F::unknown_field, 0x44a958);
    failure("action-unknown", false, [](auto& f) { f.table->records[0].action.reset(); }, F::unknown_field, 0x44a975);
    failure("action-null", false, [](auto& f) { f.table->records[0].action = AnimationResourceReference{}; }, F::null_resource, 0x44a975);
    failure("action-unresolved", false, [](auto& f) { f.resources.actions.erase(0); }, F::unresolved_resource, 0x44a975);
    failure("motion-unknown", false, [](auto& f) { f.resources.actions[0].motion.reset(); }, F::unknown_field, 0x44a975);
    failure("motion-null", false, [](auto& f) { f.resources.actions[0].motion = AnimationResourceReference{}; }, F::null_resource, 0x44a975);
    failure("motion-unresolved", false, [](auto& f) { f.resources.motions.erase(0); }, F::unresolved_resource, 0x44a975);
    failure("frame-count", false, [](auto& f) { f.resources.motions[0].frame_count.reset(); }, F::unknown_field, 0x44a975);
    failure("transition", false, [](auto& f) { f.player.animation_mode_at_120 = 2; f.table->records[0].transition_step.reset(); }, F::unknown_field, 0x44a921);
    failure("target-mode", false, [](auto& f) { f.player.animation_mode_at_120 = 2; f.table->records[0].transition_step = 1; f.table->records[0].mode.reset(); }, F::unknown_field, 0x44a93b);
    failure("next", false, [](auto& f) { f.player.animation_mode_at_120 = 1; f.table->records[0].transition_step = 1; f.table->records[0].next.reset(); }, F::unknown_field, 0x44a90c);
    failure("release-count", false, [](auto& f) { f.player.animation_mode_at_120 = 14; f.table->records[0].object_count.reset(); }, F::unknown_field, 0x43fa00);
    for (unsigned mode : {9U, 10U, 11U}) {
        const auto boundary = mode == 9 ? 0x44aa00U : mode == 10 ? 0x44aaa5U : 0x44ab52U;
        failure("arena-" + std::to_string(mode), false, [=](auto& f) { f.player.animation_mode_at_120 = mode; f.runtime.arena = nullptr; }, F::missing_arena, boundary);
        failure("allocation-" + std::to_string(mode), false, [=](auto& f) { f.player.animation_mode_at_120 = mode; f.view.player_allocation.reset(); }, F::unresolved_resource, boundary);
        failure("null-alias-" + std::to_string(mode), false, [=](auto& f) { f.player.animation_mode_at_120 = mode; require(bool(f.arena.set_reference(f.work, mode == 9 ? 0x138 : 0x13c, {})), "null alias"); }, F::null_resource, boundary);
        failure("foreign-alias-" + std::to_string(mode), false, [=](auto& f) { f.player.animation_mode_at_120 = mode; require(bool(f.arena.set_reference(f.work, mode == 9 ? 0x138 : 0x13c, f.arena.allocate(64).work)), "foreign alias"); }, F::unresolved_resource, boundary);
        failure("wrong-offset-" + std::to_string(mode), false, [=](auto& f) { f.player.animation_mode_at_120 = mode; require(bool(f.arena.set_reference_view(f.work, mode == 9 ? 0x138 : 0x13c, f.work, 4)), "bad alias offset"); }, F::unresolved_resource, boundary);
    }
    failure("charge-unknown", false, [](auto& f) { f.player.animation_mode_at_120 = 10; f.player.source_charge_at_0.reset(); }, F::unknown_field, 0x44aaa5);
    failure("selected-object", true, [](auto& f) { f.resources.actions[0].object.reset(); }, F::unknown_field, 0x43f2c8);
    failure("selected-object-null", true, [](auto& f) { f.resources.actions[0].object = SceneResourceReference{}; }, F::null_resource, 0x439fe2);
    failure("object-map", true, [](auto& f) { f.resources.objects = nullptr; }, F::unresolved_resource, 0x439fe2);
    failure("object-record", true, [](auto& f) { f.objects.clear(); }, F::unresolved_resource, 0x439fe2);
    failure("child", true, [](auto& f) { f.objects[0].child.reset(); }, F::unknown_field, 0x439fe2);
    failure("sibling", true, [](auto& f) { f.objects[0].sibling.reset(); }, F::unknown_field, 0x43a002);
    failure("next-motion-type", true, [](auto& f) { f.resources.motions[0].type.reset(); }, F::unknown_field, 0x43f2e7);
    failure("next-motion-data", true, [](auto& f) { f.resources.motions[0].data.reset(); }, F::unknown_field, 0x43f2eb);
    failure("old-motion-type", true, [](auto& f) { f.resources.motions[1].type.reset(); }, F::unknown_field, 0x43f2f6);
    failure("old-motion-data", true, [](auto& f) { f.resources.motions[1].data.reset(); }, F::unknown_field, 0x43f2fe);
    failure("allocated-action", true, [](auto& f) { f.tail.buffer_at_148.reset(); }, F::unknown_field, 0x43f2b3);
    failure("allocated-action-null", true, [](auto& f) { f.tail.buffer_at_148 = std::shared_ptr<PlayerAnimationAllocatedAction>{}; }, F::null_resource, 0x43f2d4);
    failure("allocated-motion", true, [](auto& f) { f.action->motion.reset(); }, F::unknown_field, 0x43f311);
    failure("allocated-motion-null", true, [](auto& f) { f.action->motion = std::shared_ptr<PlayerAnimationAllocatedMotion>{}; }, F::null_resource, 0x43f314);
    failure("allocated-data", true, [](auto& f) { f.motion->data.reset(); }, F::unknown_field, 0x43f314);
    failure("allocated-data-null", true, [](auto& f) { f.motion->data = TaskWorkHandle{}; }, F::null_resource, 0x43f386);
    failure("nodes-null", true, [](auto& f) { f.resources.motions[1].data = AnimationResourceReference{}; }, F::null_resource, 0x43f35c);
    failure("nodes-unresolved", true, [](auto& f) { f.resources.node_arrays.erase(1); }, F::unresolved_resource, 0x43f35c);
    failure("node-row", true, [](auto& f) { f.resources.node_arrays[1].clear(); }, F::unresolved_resource, 0x43f35c);
    failure("node-key", true, [](auto& f) { f.resources.node_arrays[1][0].keys[0].reset(); }, F::unknown_field, 0x43f35c);
    failure("key-unresolved", true, [](auto& f) { f.resources.key_arrays.erase(3); }, F::unresolved_resource, 0x43f398);
    failure("node-count", true, [](auto& f) { f.resources.node_arrays[1][0].counts[0].reset(); }, F::unknown_field, 0x43f398);
    failure("raw-scale-pointer", true, [](auto& f) { f.resources.motions[1].type = 7; f.resources.node_arrays[1][0].scale_pointer_bits_at_8.reset(); }, F::unknown_field, 0x43f39c);
    failure("key-backing-null", true, [](auto& f) { f.resources.key_arrays[3].bytes.reset(); }, F::unavailable_key, 0x43f398);
    failure("key-frame-extent", true, [](auto& f) { f.resources.key_arrays[3].byte_extent = 0; }, F::unavailable_key, 0x43f398);
    failure("key-next-frame", true, [](auto& f) { f.resources.key_arrays[3].byte_extent = 16; }, F::unavailable_key, 0x43f3cc);
    failure("key-axis-y", true, [](auto& f) { f.tail.frame_at_130 = 0; f.resources.key_arrays[3].byte_extent = 8; }, F::unavailable_key, 0x43f400);
    failure("key-axis-z", true, [](auto& f) { f.tail.frame_at_130 = 0; f.resources.key_arrays[3].byte_extent = 12; }, F::unavailable_key, 0x43f406);
    failure("key-offset", true, [](auto& f) { f.resources.key_arrays[3].offset = 33; }, F::unavailable_key, 0x43f398);
    failure("incoming-key", true, [](auto& f) { f.resources.key_arrays[0].byte_extent = 4; }, F::unavailable_key, 0x43f4a0);
    failure("hierarchy-cap", true, [](auto& f) { f.objects[0].child = SceneResourceReference{0}; }, F::object_stack_overflow, 0x43941e);
    failure("blend-table-unknown", true, [](auto& f) { f.tail.table_at_140.reset(); }, F::unknown_field, 0x43f2c5);
    failure("blend-table-null", true, [](auto& f) { f.tail.table_at_140 = std::shared_ptr<const PlayerAnimationTable>{}; }, F::null_resource, 0x43f2c5);
    failure("blend-row", true, [](auto& f) { f.table->records.erase(0); }, F::unresolved_resource, 0x43f2c5);
    failure("blend-count", true, [](auto& f) { f.table->records[0].object_count.reset(); }, F::unknown_field, 0x43f32b);
    failure("blend-arena", true, [](auto& f) { f.runtime.arena = nullptr; }, F::missing_arena, 0x43f370);
    for (unsigned channel = 0; channel < 3; ++channel) {
        const auto base = channel == 0 ? 0x43f35cU : channel == 1 ? 0x43f507U : 0x43f766U;
        const auto incoming = channel == 0 ? 0x43f49aU : channel == 1 ? 0x43f687U : 0x43f8ceU;
        const auto sample = channel == 0 ? 0x43f398U : channel == 1 ? 0x43f54cU : 0x43f7c0U;
        const auto first = channel == 0 ? 0x43f4a0U : channel == 1 ? 0x43f68eU : 0x43f8d2U;
        const auto setup = [=](auto& f) {
            if (channel == 2) {
                f.resources.motions[0].type = 7;
                f.resources.motions[1].type = 7;
                f.resources.node_arrays[1][0].scale_pointer_bits_at_8 = 2;
            }
        };
        failure("channel-old-key-" + std::to_string(channel), true, [=](auto& f) { setup(f); f.resources.node_arrays[1][0].keys[channel].reset(); }, F::unknown_field, base);
        failure("channel-new-key-" + std::to_string(channel), true, [=](auto& f) { setup(f); f.resources.node_arrays[0][0].keys[channel].reset(); }, F::unknown_field, incoming);
        failure("channel-old-count-" + std::to_string(channel), true, [=](auto& f) { setup(f); f.resources.node_arrays[1][0].counts[channel].reset(); }, F::unknown_field, sample);
        failure("channel-old-backing-" + std::to_string(channel), true, [=](auto& f) { setup(f); f.resources.key_arrays[3 + channel].bytes.reset(); }, F::unavailable_key, sample);
        failure("channel-new-backing-" + std::to_string(channel), true, [=](auto& f) { setup(f); f.resources.key_arrays[channel].bytes.reset(); }, F::unavailable_key, first);
        failure("channel-pose-" + std::to_string(channel), true, [=](auto& f) {
            setup(f);
            f.resources.node_arrays[1][0].keys[channel] = AnimationResourceReference{};
            if (channel == 0) f.objects[0].position.reset();
            else if (channel == 1) f.objects[0].angles.reset();
            else f.objects[0].scale.reset();
        }, F::unknown_field, sample);
    }
    failure("arena-unobserved-alias", false, [](auto& f) {
        f.player.animation_mode_at_120 = 9;
        auto fresh = f.arena.allocate(512).work;
        f.view.player_allocation = fresh;
    }, F::work_failed, 0x44aa00);
    failure("arena-released-player", false, [](auto& f) {
        f.player.animation_mode_at_120 = 9;
        require(bool(f.arena.release(f.work)), "release player fixture");
    }, F::work_failed, 0x44aa00);
}
void ordering() {
    // Unreached rows and graphs cannot be validated eagerly.
    {
        Fixture f;
        f.tail.table_at_140.reset();
        f.player.animation_mode_at_120 = 3;
        auto result = reset_player_animation_frame(f.view, f.runtime, f.fp);
        require(bool(result) && f.tail.frame_at_130 == 0.25F && f.tail.start_at_134 == 0, "reset lazy fields");
        record("reset-unreached-table", f, result);
    }
    {
        Fixture f;
        f.table->records[0].next = 0xab01;
        f.table->records[1].next = 0xcd00;
        const auto result = next_player_animation(f.view, f.runtime, 0, f.fp);
        require(bool(result) && f.player.requested_animation_at_124 == 1 && f.tail.next_at_12a == 0, "next reads low byte only");
        record("next-low-byte", f, result);
    }
    for (unsigned mode : {9U, 10U, 11U}) {
        Fixture f;
        f.player.animation_mode_at_120 = mode;
        auto first = f.update();
        require(bool(first), "first live alias");
        f.player.motion.speed.x = 6;
        f.player.source_charge_at_0 = 7;
        f.tail.frame_at_130 = 0;
        auto second = f.update();
        require(bool(second) && f.tail.frame_at_130 == (mode == 9 ? 3 : mode == 10 ? 3.5F : 7), "aliases read live canonical scalars");
        record("live-alias-" + std::to_string(mode), f, second);
    }
    {
        Fixture f;
        f.objects[0].child = SceneResourceReference{1};
        f.objects[1] = f.objects[0];
        f.objects[1].child = SceneResourceReference{};
        f.objects[0].sibling = SceneResourceReference{1};
        f.table->records[0].object_count = 3;
        for (unsigned index = 0; index < 2; ++index)
            f.resources.node_arrays[index].resize(3, f.resources.node_arrays[index][0]);
        const auto result = f.blend();
        require(bool(result), "hierarchy revisits repeated identity");
        for (unsigned node = 0; node < 3; ++node)
            require(bool(f.arena.reference(*f.motion->data, node * 16).work), "repeated node generated");
        record("hierarchy-repeats", f, result);
    }
    {
        Fixture f;
        f.resources.motions[1].data = AnimationResourceReference{};
        auto result = f.blend();
        require(!result && f.action->object->id == 0, "object publication precedes node failure");
        record("object-publication-prefix", f, result);
    }
    for (unsigned extent : {8U, 12U}) {
        Fixture f;
        f.tail.frame_at_130 = 0;
        f.resources.key_arrays[3].byte_extent = extent;
        const auto result = f.blend();
        auto output = f.arena.reference(*f.motion->data, 0).work;
        std::uint32_t frame = 99, x = 99;
        require(output && bool(f.arena.read(output, 0, &frame, 4)) && bool(f.arena.read(output, 4, &x, 4)), "retained output");
        require(!result && frame == 0 && x == 0x3f800000, "earlier stores retained on later axis failure");
        record("axis-prefix-" + std::to_string(extent), f, result);
    }
    {
        Fixture f;
        // Already-null channels retain their counts; release only clears nonnull.
        require(bool(f.arena.set_reference(*f.motion->data, 0, {})), "known null channel");
        const std::uint32_t count = 77;
        require(bool(f.arena.write(*f.motion->data, 8, &count, 4)), "old null count");
        auto result = release_animation_keys(f.view, f.runtime);
        std::uint32_t after = 0;
        require(bool(result) && bool(f.arena.read(*f.motion->data, 8, &after, 4)) && after == count, "null count retained");
        record("null-release-count", f, result);
    }
    // Source malloc headers are partial typed publications. Exhaust the real
    // arena to leave room for exactly 0, 1 or 2 rounded 8/12-byte allocations.
    for (unsigned available : {0U, 64U, 128U}) {
        Fixture f;
        const auto head = *f.arena.free_head();
        const auto blocks = f.arena.blocks();
        const auto found = std::find_if(blocks.begin(), blocks.end(),
            [=](const auto& block) { return block.offset == head; });
        require(found != blocks.end() && found->physical_next.has_value(), "remaining arena block");
        const auto bytes = *found->physical_next - head;
        auto pressure = f.arena.allocate(bytes - available - 32);
        require(bool(pressure), "controlled exhaustion");
        f.player.animation_mode_at_120 = 0;
        f.tail.buffer_at_148.reset();
        auto result = f.update();
        require(f.player.animation_mode_at_120 == 1 && f.tail.frame_at_130 == 0 && f.tail.start_at_134 == 0,
                "mode/frame publication precedes allocation failure");
        if (!available) {
            require(result.failure == PlayerAnimationFailure::null_resource && result.source_boundary == 0x44a880,
                    "null action allocation fails at first store");
            require(f.tail.buffer_at_148 && !*f.tail.buffer_at_148, "null action published");
        } else {
            auto action = *f.tail.buffer_at_148;
            require(action && action->object->id == 0 && action->motion.has_value(), "action object and motion publication");
            if (available == 64) {
                require(result.failure == PlayerAnimationFailure::null_resource && result.source_boundary == 0x44a898,
                        "null motion allocation fails at first store");
                require(!*action->motion, "null motion is known published");
            } else {
                require(bool(result), "failed calloc data allocation is published null");
                auto motion = *action->motion;
                require(motion->data && !*motion->data && motion->frame_count == 2 && motion->type == 3 && motion->input_function == 2,
                        "fully established motion header, null data");
            }
        }
        record("allocation-exhaustion-" + std::to_string(available), f, result);
    }
    {
        Fixture f;
        f.player.animation_mode_at_120 = 0;
        f.resources.actions[0].object.reset();
        auto result = f.update();
        require(f.tail.buffer_at_148 && *f.tail.buffer_at_148, "action published before source object read");
        const auto action = *f.tail.buffer_at_148;
        require(result.failure == PlayerAnimationFailure::unknown_field && result.source_boundary == 0x44a87c,
                "source object unavailable after action publication");
        require(action && !action->object && !action->motion, "malloc action fields remain unavailable before stores");
        record("malloc-partial-action", f, result);
    }
    {
        Fixture f;
        // Real existing-interface observers can reject a reached data clear.
        // Header mallocs precede this callback; its state must already be public.
        bool observed = false;
        f.arena.set_payload_transport({}, [&](auto, const auto*, auto size) {
            if (size != 111 * 24) return true;
            auto action = *f.tail.buffer_at_148;
            auto motion = *action->motion;
            require(action->object->id == 0 && motion->frame_count == 2 && motion->type == 3 &&
                    motion->input_function == 2 && !motion->data, "malloc motion partially published before calloc clear");
            observed = true;
            return false;
        });
        f.player.animation_mode_at_120 = 0;
        auto result = f.update();
        require(observed && result.failure == PlayerAnimationFailure::work_failed && result.source_boundary == 0x44a8b5,
                "rejected data initialization boundary");
        record("malloc-partial-motion", f, result);
    }
    {
        Fixture f;
        auto key = f.arena.allocate(32).work;
        auto later_key = f.arena.allocate(32).work;
        require(bool(f.arena.set_reference(*f.motion->data, 0, key)), "first key");
        require(bool(f.arena.set_reference_view(*f.motion->data, 4, later_key, 4)), "invalid later free interior");
        const std::uint32_t count = 2;
        require(bool(f.arena.write(*f.motion->data, 8, &count, 4)), "first count");
        const auto result = release_animation_keys(f.view, f.runtime);
        std::uint32_t after = 99;
        require(result.failure == PlayerAnimationFailure::unresolved_resource && result.source_boundary == 0x43fa3e,
                "later interior free boundary");
        require(key->released() && !f.arena.reference(*f.motion->data, 0).work &&
                bool(f.arena.read(*f.motion->data, 8, &after, 4)) && after == 0,
                "first channel freed and cleared before later failure");
        record("release-order-prefix", f, result);
    }
    for (unsigned length : {111U, 112U}) {
        Fixture f;
        for (unsigned index = 0; index < length; ++index) {
            f.objects[index] = f.objects[0];
            f.objects[index].child = index + 1 == length ? SceneResourceReference{} : SceneResourceReference{index + 1};
        }
        const auto result = f.blend();
        require(length == 111 ? bool(result) : result.failure == PlayerAnimationFailure::object_stack_overflow,
                "exact 111-object hierarchy capacity");
        record("hierarchy-length-" + std::to_string(length), f, result);
    }
    for (bool scale : {false, true}) {
        Fixture f;
        f.player.current_animation_at_126 = 1;
        f.resources.motions[1].type = scale ? 7 : 3;
        f.motion->data.reset();
        const auto result = blend_player_animation(f.view, f.runtime, f.fp);
        require(result.failure == PlayerAnimationFailure::unknown_field &&
                result.source_boundary == (scale ? 0x4469a0U : 0x446996U) &&
                result.nested_source_boundary == 0x43f314 && result.callee_address == 0x43f2a0,
                "nested transition boundary and caller preserved");
        record(scale ? "nested-mdata3" : "nested-mdata2", f, result);
    }
    {
        Fixture f;
        f.player.requested_animation_at_124 = 1;
        f.table->records[1].object_count = 0;
        f.motion->data = TaskWorkHandle{};
        const auto result = release_animation_keys(f.view, f.runtime);
        require(bool(result), "release reads requested row count and skips null data for zero nodes");
        record("release-requested-count-zero", f, result);
    }
    {
        PlayerPhysicsState generic;
        PlayerAnimationTail tail;
        PlayerAnimationAllocatedAction action;
        PlayerAnimationAllocatedMotion motion;
        require(!generic.animation_mode_at_120 && !generic.requested_animation_at_124 &&
                !tail.table_at_140 && !tail.timer_at_12c && !tail.frame_at_130 &&
                !action.object && !action.motion && !motion.data && !motion.type && !motion.frame_count,
                "arbitrary fixtures and malloc headers do not acquire native calloc defaults");
    }
    {
        Fixture f;
        f.player.animation_mode_at_120 = 14;
        f.table->records[0].object_count = 0;
        auto data = *f.motion->data;
        unsigned observed = 0;
        f.arena.set_metadata_writer([&](auto, auto) {
            ++observed;
            require(!f.motion->allocation->released() || data->released(), "data freed before motion");
            require(!f.action->allocation->released() || f.motion->allocation->released(), "motion freed before action");
        });
        const auto result = f.update();
        require(bool(result) && observed && data->released() && f.motion->allocation->released() &&
                f.action->allocation->released() && f.tail.buffer_at_148 && !*f.tail.buffer_at_148,
                "default release completes in source order");
        record("default-release-order", f, result);
    }
}
}
int main() {
    try {
        missingness();
        ordering();
        std::cerr << cases << " source-backed interface characterizations passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
