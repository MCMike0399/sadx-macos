#include "game/game_library_init_internal.hpp"
#include "cpu/fidelity_float.hpp"

namespace sadx::library {
namespace {
// NOTSA: named shared shutdown tail; both original call sites use this order.
void shut_down_libraries(Context &context, CleanupReason reason) {
    context.cleanup_reason = reason;
    context.shutdown_file_system();
    context.unregister_debug_text();
    context.shutdown_input();
    context.release_system_pool();
    context.destroy_heap();
    context.release_graphics();
    context.close_instance_mutex();
}
// NOTSA: retain the source compiler's signed load and unsigned correction.
// Loading the positive integer directly changes results under some x87 controls.
fidelity::ExactFloat unsigned_dimension(X87Context &arithmetic, std::uint32_t value) {
    const auto signed_value = value < 2147483648U
                                  ? static_cast<std::int64_t>(value)
                                  : static_cast<std::int64_t>(value) - 4294967296LL;
    auto retained = fidelity::ExactFloat::integer(arithmetic, signed_value);
    if (signed_value < 0)
        retained = retained + 4294967296.0f;
    return retained;
}
} // namespace
// FUNCTION: SONIC 0x781070 InitializeSonicLibrary
std::uint32_t initialize_libraries(Context &context) {
    context.capture_application_paths();
    for (std::size_t index = 0; index < 9; ++index)
        context.set_polygon_buffer_count(index, context.configured_polygon_buffer_count(index));
    context.set_windowed(context.configured_windowed());
    context.set_back_buffer_width(context.configured_back_buffer_width());
    context.set_back_buffer_height(context.configured_back_buffer_height());
    if (context.configured_high_precision_depth_request() == 1)
        context.enable_high_precision_depth();
    context.configure_application_paths();
    context.initialize_window();
    context.set_library_mode(0);
    context.initialize_graphics();
    context.create_heap();
    // Keep these reads live and in source order: preceding callees can change them.
    const auto interval = context.configured_frame_interval();
    const auto setup_argument = context.configured_screen_setup_argument();
    const auto flags = context.configured_screen_flags();
    context.configure_screen(flags, setup_argument, interval);
    context.initialize_input();
    context.register_debug_text();

    std::uint32_t attempts = 8;
    for (;;) {
        // Capacities of the source file-handle and file-entry workspaces.
        constexpr std::uint32_t kFileHandleCapacity = 8;
        constexpr std::uint32_t kFileEntryCapacity = 712;
        const auto status =
            context.initialize_file_system(kFileHandleCapacity, kFileEntryCapacity);
        // Original policy: these two statuses shut everything down before retrying.
        if (status == -23 || status == -33)
            shut_down_libraries(context, CleanupReason::Retry);
        else if (status == 0)
            break;
        if (--attempts == 0)
            break;
    }
    context.attempts_remaining = attempts;
    if (attempts == 0)
        shut_down_libraries(context, CleanupReason::Exhausted);

    const auto height = unsigned_dimension(context.arithmetic, context.viewport_height());
    // The scale store precedes the width read, including a failure at that read.
    context.scale(1.0f);
    const auto width = unsigned_dimension(context.arithmetic, context.viewport_width());
    context.aspect((height / width).rounded());
    context.focus_window();
    return attempts;
}
} // namespace sadx::library
