#include "engine/particle_pool.hpp"
#include <stdexcept>

namespace sadx::particles {
TaskHandle create_task(TaskScheduler &scheduler) {
    const auto result = scheduler.create_element(2, 4, update_task);
    if (result)
        return result.task;
    if (result.failure == TaskFailure::SourcePoolExhausted ||
        result.failure == TaskFailure::WorkFailure)
        return {};
    throw std::runtime_error("native_particle_task_creation");
}
TaskWorkHandle allocate_buffer(TaskWorkArena &arena, std::uint32_t bytes) {
    return arena.allocate(bytes).work;
}
void discard_task(TaskScheduler &scheduler, const TaskHandle &task) {
    if (!scheduler.destroy_task(task))
        throw std::runtime_error("native_particle_task_rollback");
}
void load_textures(texture_lifetime::List &) {
    throw std::runtime_error("native_particle_load_textures");
}
void update_task(TaskScheduler &, const TaskHandle &) {
    throw std::runtime_error("native_particle_update");
}
void update_custom_task(TaskScheduler &, const TaskHandle &) {
    throw std::runtime_error("native_particle_custom_update");
}
void display_task(TaskScheduler &, const TaskHandle &) {
    throw std::runtime_error("native_particle_display");
}
void destroy_task(TaskScheduler &, const TaskHandle &) {
    throw std::runtime_error("native_particle_destroy");
}
} // namespace sadx::particles
