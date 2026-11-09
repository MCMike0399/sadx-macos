#include "player/gamma_target_effects.hpp"

#include "math/ninja_math.hpp"

#include <cstdint>
#include <cstring>

namespace sadx::gamma_target_effects {
namespace {
constexpr int kReady = 1;
constexpr int kAcquire = 2;
constexpr int kLocked = 3;
constexpr int kRelease = 4;
constexpr int kClear = 5;
constexpr int kFinished = 6;
constexpr int kLastMode = 6;
constexpr std::int16_t kLockHold = 180;
constexpr double kTurnsPerRadian = 65536.0;

double image_double(std::uint64_t bits) {
    double value;
    std::memcpy(&value, &bits, sizeof(value));
    return value;
}
// Binary64 constants at 0x7dcce8 and 0x7dccc8: -1/(2π) and +1/(2π).
const double kNegativeTurn = image_double(0xbfc45f306bc4e30aULL);
const double kPositiveTurn = image_double(0x3fc45f306bc4e30aULL);

std::int32_t low_word(std::int64_t value) {
    return static_cast<std::int32_t>(static_cast<std::uint32_t>(value));
}

void consider_acquisition(LaserTarget &target, std::int8_t task_mode, Services &services) {
    if ((services.pressed(services.controller_slot()) & services.button_mask()) == 0)
        return;
    if (services.read_lock_table(task_mode) != 0)
        target.mode = kAcquire;
}

void adjust_scope(LaserTarget &target, Services &services, fidelity::Context &arithmetic) {
    const bool held = (services.held(services.controller_slot()) & services.button_mask()) != 0;
    auto scope = fidelity::ExactFloat(arithmetic, target.scope);
    scope = held ? scope + 1.0f : scope - 1.0f;
    target.scope = scope.rounded();
    if (fidelity::ExactFloat(arithmetic, target.scope).compare(0.0f) == X87Comparison::less) {
        target.scope = 0.0f;
        return;
    }
    if (fidelity::ExactFloat(arithmetic, target.scope).compare(19.0f) == X87Comparison::greater)
        target.scope = 19.0f;
}

AimPose aim_at(const LaserTarget &target, fidelity::Context &arithmetic) {
    const auto horizontal = fidelity::ExactFloat(arithmetic, target.aim_x);
    const auto vertical = fidelity::ExactFloat(arithmetic, target.aim_y);
    const auto depth = fidelity::ExactFloat(arithmetic, target.aim_z);
    // njSqrt receives the binary32 spill of z² + x². y is only in the pitch.
    const float squared = ((depth * depth) + (horizontal * horizontal)).rounded();
    const auto length =
        fidelity::ExactFloat::retained(arithmetic, signed_sqrt_extended(squared, arithmetic));
    const auto yaw = fidelity::ExactFloat::arctangent(depth, horizontal)
                         .multiply_double(kTurnsPerRadian)
                         .multiply_double(kNegativeTurn)
                         .truncated_integer();
    const auto pitch = fidelity::ExactFloat::arctangent(vertical, length)
                           .multiply_double(kTurnsPerRadian)
                           .multiply_double(kPositiveTurn)
                           .truncated_integer();
    return {target.position_x, target.position_y, target.position_z, low_word(yaw),
            low_word(pitch)};
}

void acquire(LaserTarget &target, Services &services) {
    const auto task = services.create_search_task();
    target.search = task;
    if (task != 0) {
        const auto block = services.allocate_search_block();
        if (block != 0) {
            services.attach_search_block(task, block);
            target.laser_timer = kLockHold;
            target.mode = kLocked;
        } else {
            services.destroy_task(task);
        }
    }
    if (services.event_sounds_enabled())
        services.play_lock_sound();
}

void track(LaserTarget &target, std::int8_t task_mode, Services &services,
           fidelity::Context &arithmetic) {
    const auto aim = services.create_aim_task();
    if (aim != 0)
        services.store_aim(aim, aim_at(target, arithmetic));
    if ((services.held(services.controller_slot()) & services.button_mask()) == 0) {
        target.mode = kClear;
        target.lock_combo = 0;
        target.shot_combo = 0;
    }
    target.laser_timer = static_cast<std::int16_t>(target.laser_timer - 1);
    if (target.laser_timer > 0 && services.read_lock_table(task_mode) != 0)
        return;
    target.lock_combo = 0;
    target.shot_combo = 0;
    target.mode = kRelease;
}

void release_locks(LaserTarget &target, Services &services) {
    if (target.lock_count > 0) {
        for (int index = 0; index < target.lock_count; ++index) {
            const auto subject = services.locked_subject(index);
            if (subject != 0)
                services.release_locked_target(subject);
        }
    }
    target.mode = kFinished;
    target.gun_mode = 0;
}

void clear_search(LaserTarget &target, std::int8_t task_mode, Services &services) {
    services.destroy_task(target.search);
    target.search = 0;
    target.mode = kReady;
    services.stop_lock_sound();
    consider_acquisition(target, task_mode, services);
}

void apply_mode(LaserTarget &target, std::int8_t task_mode, Services &services,
                fidelity::Context &arithmetic) {
    switch (target.mode) {
    case 0:
        target.scope = 0.0f;
        target.lock_timer = 0;
        target.distance = 0.0f;
        target.mode = kReady;
        break;
    case 1:
        consider_acquisition(target, task_mode, services);
        break;
    case 2:
        acquire(target, services);
        break;
    case 3:
        track(target, task_mode, services, arithmetic);
        break;
    case 4:
        release_locks(target, services);
        break;
    case 5:
    case 6:
        clear_search(target, task_mode, services);
        break;
    default:
        break;
    }
}
} // namespace

void update(LaserTarget &target, std::int8_t task_mode, Services &services,
            fidelity::Context &arithmetic) {
    const auto selector = static_cast<std::uint32_t>(static_cast<std::int32_t>(target.mode));
    if (selector <= kLastMode)
        apply_mode(target, task_mode, services, arithmetic);
    adjust_scope(target, services, arithmetic);
}
} // namespace sadx::gamma_target_effects
