#include "game/game_library_init_internal.hpp"

namespace sadx::library {
namespace {
// NOTSA: plain borrowed native owners for the shared initialization algorithm.
struct NativeContext final : Context {
    explicit NativeContext(Runtime &runtime) : Context(runtime.arithmetic), runtime(runtime) {}
    Runtime &runtime;
    ApplicationPaths captured_paths{};
    void capture_application_paths() override { captured_paths = runtime.configuration.paths; }
    std::uint32_t configured_polygon_buffer_count(std::size_t index) override {
        return runtime.configuration.polygon_buffer_counts.at(index);
    }
    void set_polygon_buffer_count(std::size_t index, std::uint32_t count) override {
        runtime.state.polygon_buffer_counts.at(index) = count;
    }
    std::uint32_t configured_windowed() override { return runtime.configuration.windowed; }
    void set_windowed(std::uint32_t value) override { runtime.state.windowed = value; }
    std::uint32_t configured_back_buffer_width() override { return runtime.configuration.back_buffer_width; }
    void set_back_buffer_width(std::uint32_t value) override { runtime.state.back_buffer_width = value; }
    std::uint32_t configured_back_buffer_height() override { return runtime.configuration.back_buffer_height; }
    void set_back_buffer_height(std::uint32_t value) override { runtime.state.back_buffer_height = value; }
    std::uint32_t configured_high_precision_depth_request() override {
        return runtime.configuration.high_precision_depth_request;
    }
    std::uint32_t configured_screen_flags() override { return runtime.configuration.screen_flags; }
    std::uint32_t configured_screen_setup_argument() override { return runtime.configuration.screen_setup_argument; }
    std::uint32_t configured_frame_interval() override { return runtime.configuration.frame_interval; }
    std::uint32_t viewport_height() override { return runtime.viewport.height; }
    std::uint32_t viewport_width() override { return runtime.viewport.width; }
    void scale(float value) override { runtime.state.scale = value; }
    void aspect(float value) override { runtime.state.aspect = value; }
    void enable_high_precision_depth() override { runtime.services.enable_high_precision_depth(); }
    void configure_application_paths() override { runtime.services.configure_application_paths(captured_paths); }
    void initialize_window() override { runtime.services.initialize_window(); }
    void initialize_graphics() override { runtime.services.initialize_graphics(); }
    void create_heap() override { runtime.services.create_heap(); }
    void initialize_input() override { runtime.services.initialize_input(); }
    void register_debug_text() override { runtime.services.register_debug_text(); }
    void shutdown_file_system() override { runtime.services.shutdown_file_system(); }
    void unregister_debug_text() override { runtime.services.unregister_debug_text(); }
    void shutdown_input() override { runtime.services.shutdown_input(); }
    void release_system_pool() override { runtime.services.release_system_pool(); }
    void destroy_heap() override { runtime.services.destroy_heap(); }
    void release_graphics() override { runtime.services.release_graphics(); }
    void close_instance_mutex() override { runtime.services.close_instance_mutex(); }
    void focus_window() override { runtime.services.focus_window(); }
    void set_library_mode(std::uint32_t mode) override {
        runtime.services.set_library_mode(mode);
    }
    void configure_screen(std::uint32_t flags, std::uint32_t setup_argument,
                          std::uint32_t interval) override {
        runtime.services.configure_screen(flags, setup_argument, interval);
    }
    std::int32_t initialize_file_system(std::uint32_t handles, std::uint32_t entries) override {
        return runtime.services.initialize_file_system(handles, entries);
    }
};
} // namespace
// NOTSA: native composition has no reference headers, optional facts or executable data.
std::uint32_t initialize_libraries(Runtime &runtime) {
    NativeContext context(runtime);
    return initialize_libraries(context);
}
} // namespace sadx::library
