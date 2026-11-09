#pragma once

#include "cpu/fidelity_float.hpp"

#include <cstdint>

namespace sadx::gamma_target_effects {
// NOTSA: the E102WK fields this update reads and writes, in source order.
// Task and sound owners stay behind Services so the product and the oracle
// share one body.
struct LaserTarget {
    std::int8_t mode = 0;
    std::int8_t gun_mode = 0;
    std::int8_t lock_count = 0;
    std::int16_t lock_combo = 0;
    std::int16_t shot_combo = 0;
    std::int16_t laser_timer = 0;
    std::int16_t lock_timer = 0;
    float position_x = 0, position_y = 0, position_z = 0;
    float aim_x = 0, aim_y = 0, aim_z = 0;
    float distance = 0;
    float scope = 0;
    // Zero is the source null task pointer. A failed allocation keeps the id;
    // finishing the search stores zero after DestroyTask.
    std::uintptr_t search = 0;
};

struct AimPose {
    float x = 0, y = 0, z = 0;
    std::int32_t yaw = 0, pitch = 0;
};

// Callees the product already owns return their real results. Unrecovered
// callees fail inside the product service at the call, after earlier stores.
struct Services {
    virtual ~Services() = default;
    // taskwk +8. Scope, lock pressed and lock held all index with this byte.
    virtual std::uint8_t controller_slot() = 0;
    virtual std::uint32_t button_mask() = 0;
    virtual std::uint32_t held(std::uint8_t slot) = 0;
    virtual std::uint32_t pressed(std::uint8_t slot) = 0;
    virtual bool event_sounds_enabled() = 0;
    virtual std::uintptr_t create_search_task() = 0;
    virtual std::uintptr_t allocate_search_block() = 0;
    virtual void attach_search_block(std::uintptr_t task, std::uintptr_t block) = 0;
    virtual void destroy_task(std::uintptr_t task) = 0;
    virtual void play_lock_sound() = 0;
    virtual void stop_lock_sound() = 0;
    virtual std::uintptr_t create_aim_task() = 0;
    virtual void store_aim(std::uintptr_t task, const AimPose &pose) = 0;
    // STUB owner: SONIC 0x47f980. Reads the task mode byte, then one table byte.
    virtual std::int32_t read_lock_table(std::int8_t task_mode) = 0;
    // E102WK+0xf4 list. Zero means the auxiliary pointer was null and the
    // source skips the call. A null task pointer is a source fault.
    virtual std::uintptr_t locked_subject(int index) = 0;
    // STUB owner: SONIC 0x4cf060. The resolved auxiliary+8 value.
    virtual void release_locked_target(std::uintptr_t subject) = 0;
};

// FUNCTION: SONIC 0x4c4da0 GammaUpdateTargetEffects
// task_mode is the gamma taskwk mode byte consumed by the lock-table read.
void update(LaserTarget &target, std::int8_t task_mode, Services &services,
            fidelity::Context &arithmetic);
} // namespace sadx::gamma_target_effects
