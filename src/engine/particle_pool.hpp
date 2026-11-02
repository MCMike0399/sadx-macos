#pragma once

#include "cpu/fidelity_float.hpp"
#include "engine/native_texture_lifetime.hpp"
#include "engine/task.hpp"
#include <stdexcept>

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
// These names describe the source slot roles. Two uninterpreted words between
// velocity and animation are retained by the arena and never read by update.
enum class Field : std::uint32_t {
    PositionX = 0, PositionY = 4, PositionZ = 8,
    VelocityX = 12, VelocityY = 16, VelocityZ = 20,
    AnimationTime = 32, AnimationSpeed = 36, FrameIndex = 40, LoopCount = 44
};
enum class Operation {
    Work, FrameRead, ActiveIndexRead, ParticleRead, ParticleStore, FreeIndexStore,
    ActiveIndexStore, CustomUpdate, ArithmeticBinding, DisplayBinding, Release
};
// NOTSA: report the actual failed access without embedding guest PCs in game code.
struct Failure : std::runtime_error {
    Operation operation;
    Field field;
    TaskWorkFailure cause;
    Failure(Operation, Field = Field::PositionX,
            TaskWorkFailure = TaskWorkFailure::None);
};
// NOTSA: borrowed slot view. Each access reaches the
// canonical arena separately, retaining source reads, stores and failure prefixes.
struct Particle {
    TaskWorkArena &arena;
    TaskWorkHandle storage;
    std::uint32_t byte_offset;
    float scalar(Field) const;
    void scalar(Field, float);
    std::uint32_t word(Field) const;
    void word(Field, std::uint32_t);
};
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
// FUNCTION: SONIC 0x4cc080 UpdateParticlePool
void update_task(TaskScheduler &, const TaskHandle &);
// FUNCTION: SONIC 0x4cc150 UpdateCustomParticlePool
void update_custom_task(TaskScheduler &, const TaskHandle &);
// FUNCTION: SONIC 0x4cc040 DisplayParticlePool
void display_task(TaskScheduler &, const TaskHandle &);
// FUNCTION: SONIC 0x4cc1e0 DestroyParticlePool
void destroy_task(TaskScheduler &, const TaskHandle &);

// NOTSA: Composition supplies the existing arithmetic and rendering owners through the
// scheduler's callback context. Empty pools consume neither binding.
struct Binding : TaskCallbackContext {
    fidelity::Context &arithmetic;
    std::function<void(Particle &, Frame *)> display;
    explicit Binding(fidelity::Context &value) : arithmetic(value) {}
};
// STUB: SONIC 0x4cc540 DisplayParticle awaiting shared renderer composition
void display_particle(TaskScheduler &, Particle &, Frame *);
// FUNCTION: SONIC 0x4cc470 UpdateParticleAnimation
std::int32_t update_animation(Particle &, Frame *, fidelity::Context &);
// NOTSA: release seam permits source-ordered allocator observations and reentry.
void release_buffer(TaskWorkArena &, const TaskWorkHandle &);

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
