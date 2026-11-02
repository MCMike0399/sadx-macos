#include "runtime/native_sonic_context.hpp"
#include "runtime/native_event_actions_context.hpp"

namespace sadx::sonic {
void bind_event_work(Context &context) {
    const auto primary = native::capture_event_primary(context.task);
    const auto arena = context.scheduler.work_arena();
    if (!arena)
        throw Unavailable("sonic_task_arena");
    context.read_event_mode = [arena, primary] {
        return event_actions::move_mode(*arena, primary.allocation);
    };
    context.write_event_mode = [arena, primary](std::uint8_t mode) {
        event_actions::set_move_mode(*arena, primary.allocation, mode);
    };
    context.read_event_link = [arena, primary] {
        return event_actions::has_path(*arena, primary.allocation);
    };
}
} // namespace sadx::sonic
