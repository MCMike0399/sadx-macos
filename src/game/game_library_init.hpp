#pragma once
#include "cpu/x87_math.hpp"
#include <array>
#include <cstdint>
#include <string_view>

namespace sadx::library {
// NOTSA: borrowed text resources copied by the original library path setup.
struct ApplicationPaths {
    std::string_view company;
    std::string_view application;
    std::string_view system_root;
    std::string_view backup_root;
};
// NOTSA: startup values consumed by library initialization, named from its readers.
struct Configuration {
    ApplicationPaths paths;
    std::array<std::uint32_t, 9> polygon_buffer_counts;
    std::uint32_t windowed, back_buffer_width, back_buffer_height;
    std::uint32_t high_precision_depth_request;
    std::uint32_t screen_flags, screen_setup_argument, frame_interval;
};
struct State {
    std::array<std::uint32_t, 9> polygon_buffer_counts;
    std::uint32_t windowed, back_buffer_width, back_buffer_height;
    float scale, aspect;
};
struct Viewport {
    std::uint32_t width, height;
};
// NOTSA: named startup callees; native hosts own their windows, buffers and heaps.
// Implementations can inspect or mutate the borrowed configuration and state.
// An unavailable implementation must throw; it must not report a guessed success.
struct Services {
    virtual ~Services() = default;
    virtual void enable_high_precision_depth() = 0;
    virtual void configure_application_paths(const ApplicationPaths &) = 0;
    virtual void initialize_window() = 0;
    virtual void set_library_mode(std::uint32_t) = 0;
    virtual void initialize_graphics() = 0;
    virtual void create_heap() = 0;
    virtual void configure_screen(std::uint32_t flags, std::uint32_t setup_argument,
                                  std::uint32_t interval) = 0;
    virtual void initialize_input() = 0;
    virtual void register_debug_text() = 0;
    // Meaning of nonzero statuses is not established. The caller retains the
    // original retry policy, including its two cleanup-triggering error codes.
    virtual std::int32_t initialize_file_system(std::uint32_t handle_capacity,
                                               std::uint32_t entry_capacity) = 0;
    virtual void shutdown_file_system() = 0;
    virtual void unregister_debug_text() = 0;
    virtual void shutdown_input() = 0;
    virtual void release_system_pool() = 0;
    virtual void destroy_heap() = 0;
    virtual void release_graphics() = 0;
    virtual void close_instance_mutex() = 0;
    virtual void focus_window() = 0;
};
// NOTSA: borrowed native owners; viewport values are read after device setup.
struct Runtime {
    const Configuration &configuration;
    State &state;
    const Viewport &viewport;
    Services &services;
    X87Context &arithmetic;
};
// Returns the attempts remaining; zero still follows the original shutdown/
// viewport/focus tail. Exceptions retain stores completed before the failed call.
std::uint32_t initialize_libraries(Runtime &);
} // namespace sadx::library
