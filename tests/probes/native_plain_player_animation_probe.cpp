// Same frozen C ABI and fixture vectors, through plain native resources.
#include "guest/animation_phase_reference.hpp"
#include "player/animation/animation.hpp"
#include "player/player_physics.hpp"
#include <cstring>
namespace {
float f(std::uint32_t b) {
    float x;
    std::memcpy(&x, &b, 4);
    return x;
}
std::uint32_t bits(float f) {
    std::uint32_t b;
    std::memcpy(&b, &f, 4);
    return b;
}
} // namespace
extern "C" int sadx_native_player_animation(const float *constants, const std::uint32_t *in,
                                            std::uint32_t *out) {
    if (!constants || !in || !out)
        return 1;
    sadx::PlayerPhysicsState player{};
    player.animation_mode_at_120 = static_cast<std::int16_t>(in[0]);
    player.requested_animation_at_124 = static_cast<std::uint16_t>(in[1]);
    player.current_animation_at_126 = static_cast<std::uint16_t>(in[2]);
    player.previous_animation_at_128 = static_cast<std::uint16_t>(in[3]);
    player.motion.speed.x = f(in[9]);
    player.source_charge_at_0 = f(in[10]);
    auto &tail = player.animation_tail;
    tail.next_at_12a = static_cast<std::uint16_t>(in[4]);
    tail.timer_at_12c = static_cast<std::uint16_t>(in[5]);
    tail.flags_at_12e = static_cast<std::uint16_t>(in[6]);
    tail.frame_at_130 = f(in[7]);
    tail.start_at_134 = f(in[8]);
    auto table = std::make_shared<sadx::animation::Table>();
    sadx::animation::Resources resources;
    for (unsigned i = 0; i < 4; ++i) {
        auto &record = table->records[i];
        record.action = {sadx::animation::ResourceStatus::Loaded, i, {}};
        record.object_count = 0;
        record.mode = static_cast<sadx::animation::Mode>(static_cast<std::int8_t>(in[13 + i]));
        record.next = static_cast<std::uint16_t>(in[17 + i]);
        record.transition_step = f(in[21 + i]);
        record.frame_step = f(in[25 + i]);
        resources.actions[i].object =
            sadx::SceneResourceReference{i < 2 ? std::size_t(1) : std::size_t(2)};
        resources.actions[i].motion = sadx::animation::ResourceReference{i};
        resources.motions[i].frame_count = in[29 + i];
        resources.motions[i].type = static_cast<std::uint16_t>(in[33 + i]);
        resources.motions[i].input_function = 2;
    }
    tail.table_at_140 = table;
    sadx::TaskWorkArena arena;
    auto action = std::make_shared<sadx::PlayerAnimationAllocatedAction>();
    action->allocation = arena.allocate(8).work;
    action->object = sadx::SceneResourceReference{1};
    auto motion = std::make_shared<sadx::PlayerAnimationAllocatedMotion>();
    motion->allocation = arena.allocate(12).work;
    motion->data = sadx::TaskWorkHandle{};
    motion->frame_count = 2;
    motion->type = 3;
    motion->input_function = 2;
    action->motion = motion;
    tail.buffer_at_148 = action;

    sadx::animation::Runtime runtime{&arena, resources};
    sadx::X87Context fp(static_cast<std::uint16_t>(in[11]));
    auto player_work = arena.allocate(512).work;
    if (!arena.set_reference_view(player_work, 0x138, player_work, 0x38) ||
        !arena.set_reference(player_work, 0x13c, player_work))
        return 3;
    sadx::animation::View view{player, player_work};
    sadx::animation::Outcome result;
    if (in[12] == 0)
        result = sadx::animation::update(view, runtime, fp);
    else if (in[12] == 1)
        result = sadx::animation::reset_frame(view, runtime, fp);
    else
        result = sadx::animation::next(view, runtime, static_cast<std::uint16_t>(in[1]), fp);
    out[0] = static_cast<std::uint16_t>(*player.animation_mode_at_120);
    out[1] = *player.requested_animation_at_124;
    out[2] = *player.current_animation_at_126;
    out[3] = *player.previous_animation_at_128;
    out[4] = *tail.next_at_12a;
    out[5] = *tail.timer_at_12c;
    out[6] = *tail.flags_at_12e;
    out[7] = bits(*tail.frame_at_130);
    out[8] = bits(*tail.start_at_134);
    out[9] = fp.exception_flags();
    out[10] = fp.control_word();
    out[11] = static_cast<std::uint32_t>(result.failure);
    out[12] = sadx::animation_source_boundary(result.phase);
    out[13] = result.completed;
    out[14] = result.wrap_iterations;
    return 0;
}
