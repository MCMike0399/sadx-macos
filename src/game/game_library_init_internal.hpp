#pragma once
#include "game/game_library_init.hpp"
#include <cstddef>

namespace sadx::library {
enum class CleanupReason { Retry, Exhausted };
// NOTSA: lazy reference facts and immediate native stores share one algorithm.
// Availability checks and source diagnostics belong to the reference adapter.
struct Context {
    explicit Context(X87Context &arithmetic) : arithmetic(arithmetic) {}
    virtual ~Context() = default;
    X87Context &arithmetic;
    std::uint32_t attempts_remaining = 0;
    CleanupReason cleanup_reason = CleanupReason::Retry;
    virtual void capture_application_paths() = 0;
    virtual std::uint32_t configured_polygon_buffer_count(std::size_t) = 0;
    virtual void set_polygon_buffer_count(std::size_t, std::uint32_t) = 0;
    virtual std::uint32_t configured_windowed() = 0;
    virtual void set_windowed(std::uint32_t) = 0;
    virtual std::uint32_t configured_back_buffer_width() = 0;
    virtual void set_back_buffer_width(std::uint32_t) = 0;
    virtual std::uint32_t configured_back_buffer_height() = 0;
    virtual void set_back_buffer_height(std::uint32_t) = 0;
    virtual std::uint32_t configured_high_precision_depth_request() = 0;
    virtual std::uint32_t configured_screen_flags() = 0;
    virtual std::uint32_t configured_screen_setup_argument() = 0;
    virtual std::uint32_t configured_frame_interval() = 0;
    virtual std::uint32_t viewport_height() = 0;
    virtual std::uint32_t viewport_width() = 0;
    virtual void scale(float) = 0;
    virtual void aspect(float) = 0;
    virtual void enable_high_precision_depth() = 0;
    virtual void configure_application_paths() = 0;
    virtual void initialize_window() = 0;
    virtual void set_library_mode(std::uint32_t) = 0;
    virtual void initialize_graphics() = 0;
    virtual void create_heap() = 0;
    virtual void configure_screen(std::uint32_t, std::uint32_t, std::uint32_t) = 0;
    virtual void initialize_input() = 0;
    virtual void register_debug_text() = 0;
    virtual std::int32_t initialize_file_system(std::uint32_t, std::uint32_t) = 0;
    virtual void shutdown_file_system() = 0;
    virtual void unregister_debug_text() = 0;
    virtual void shutdown_input() = 0;
    virtual void release_system_pool() = 0;
    virtual void destroy_heap() = 0;
    virtual void release_graphics() = 0;
    virtual void close_instance_mutex() = 0;
    virtual void focus_window() = 0;
};
std::uint32_t initialize_libraries(Context &);
} // namespace sadx::library
