#pragma once

#include "cpu/fidelity_float.hpp"
#include "engine/native_texture_lifetime.hpp"
#include "engine/task.hpp"

namespace sadx::particles {
// One source animation record. The sentinel is a negative texture index.
struct Frame {
    std::int16_t texture_index = -1;
    std::uint8_t flags = 0, padding = 0;
    std::int32_t global_index = 0;
    float width = 0, height = 0, center_x = 0, center_y = 0;
    std::array<float, 5> parameters{};
};
struct Parameters {
    float drag, vertical_acceleration;
};
struct Particle;
using Update = std::int32_t (*)(Particle &, const Parameters &, Frame *);
// Typed view of the real task.awp allocation. Buffers retain their arena bytes,
// including uninitialized active-index and particle storage.
struct Work {
    TaskWorkHandle allocation;
    std::uint16_t active_count = 0, capacity = 0;
    TaskWorkHandle free_indices, active_indices, particles;
    Update update = nullptr;
    Parameters parameters{};
    Frame *frames = nullptr;
};

// STUB: SONIC 0x4228e0 NeonuLoadTexture not reached with Gamma's zero flags
void load_textures(texture_lifetime::List &);
// STUB: SONIC 0x4cc080 UpdateParticlePool awaiting update recovery
void update_task(TaskScheduler &, const TaskHandle &);
// STUB: SONIC 0x4cc150 UpdateCustomParticlePool awaiting custom update recovery
void update_custom_task(TaskScheduler &, const TaskHandle &);
// STUB: SONIC 0x4cc040 DisplayParticlePool awaiting display recovery
void display_task(TaskScheduler &, const TaskHandle &);
// STUB: SONIC 0x4cc1e0 DestroyParticlePool awaiting buffer cleanup recovery
void destroy_task(TaskScheduler &, const TaskHandle &);

// NOTSA: allocation seams preserve reentry and original failure prefixes in tests;
// the product binds these directly to its existing scheduler and arena.
TaskHandle create_task(TaskScheduler &);
TaskWorkHandle allocate_buffer(TaskWorkArena &, std::uint32_t bytes);
void discard_task(TaskScheduler &, const TaskHandle &);

// FUNCTION: SONIC 0x4cc330 BindParticleFrames
void bind_frames(texture_lifetime::List &, Frame *, std::uint8_t flags, fidelity::Context &);
// FUNCTION: SONIC 0x4cc210 CreateParticlePool
TaskHandle create_pool(TaskScheduler &, TaskWorkArena &, std::int32_t count,
                       const Parameters &, Frame *, Update);
} // namespace sadx::particles
