#include "game/game_library_init.hpp"
#include <iostream>
#include <stdexcept>

namespace {
using namespace sadx;
void require(bool value, const char *message) {
    if (!value)
        throw std::runtime_error(message);
}
struct Services final : library::Services {
    library::Configuration &configuration;
    library::State &state;
    library::Viewport &viewport;
    bool stop_at_screen = false, zero_width = false, focused = false;
    bool paths_seen = false, screen_seen = false;
    Services(library::Configuration &configuration, library::State &state,
             library::Viewport &viewport)
        : configuration(configuration), state(state), viewport(viewport) {}
    void enable_high_precision_depth() override {
        require(state.polygon_buffer_counts[0] == 1 && state.windowed == 1 &&
                    state.back_buffer_width == 640 && state.back_buffer_height == 480,
                "configuration stores precede the first callback");
        configuration.polygon_buffer_counts[0] = 999;
        configuration.paths.application = "replacement";
        configuration.screen_flags = 19;
        configuration.screen_setup_argument = 8;
        configuration.frame_interval = 2;
    }
    void configure_application_paths(const library::ApplicationPaths &paths) override {
        require(paths.application == "original", "text identities captured before callbacks");
        paths_seen = true;
    }
    void initialize_window() override {}
    void set_library_mode(std::uint32_t mode) override { require(mode == 0, "initial mode"); }
    void initialize_graphics() override { viewport = {640, 480}; }
    void create_heap() override {}
    void configure_screen(std::uint32_t flags, std::uint32_t setup,
                          std::uint32_t interval) override {
        require(paths_seen && flags == 19 && setup == 8 && interval == 2,
                "screen configuration remains live across earlier callbacks");
        screen_seen = true;
        if (stop_at_screen)
            throw std::runtime_error("controlled screen failure");
    }
    void initialize_input() override {}
    void register_debug_text() override {}
    std::int32_t initialize_file_system(std::uint32_t handles,
                                       std::uint32_t entries) override {
        require(screen_seen && handles == 8 && entries == 712, "source workspace capacities");
        viewport = {zero_width ? 0U : 1280U, 720};
        return 0;
    }
    void shutdown_file_system() override {}
    void unregister_debug_text() override {}
    void shutdown_input() override {}
    void release_system_pool() override {}
    void destroy_heap() override {}
    void release_graphics() override {}
    void close_instance_mutex() override {}
    void focus_window() override {
        require(state.scale == 1 && state.aspect == 0.5625f,
                "final focus observes the completed viewport stores");
        focused = true;
    }
};
void check(std::uint16_t control, unsigned failure) {
    library::Configuration configuration{{"company", "original", "system", "backup"},
                                         {1, 2, 3, 4, 5, 6, 7, 8, 9},
                                         1, 640, 480, 1, 49, 5, 3};
    library::State state{{}, 0, 0, 0, 12, 13};
    library::Viewport viewport{0, 0};
    Services services(configuration, state, viewport);
    services.stop_at_screen = failure == 1;
    services.zero_width = failure == 2;
    X87Context arithmetic(control);
    library::Runtime runtime{configuration, state, viewport, services, arithmetic};
    bool stopped = false;
    try {
        require(library::initialize_libraries(runtime) == 8, "first attempt succeeds");
    } catch (const X87Error &) {
        require(failure == 2, "only the controlled unmasked divide stops arithmetic");
        stopped = true;
    } catch (const std::runtime_error &error) {
        require(failure == 1 && std::string(error.what()) == "controlled screen failure",
                "only the controlled screen callback stops initialization");
        stopped = true;
    }
    require(stopped == (failure != 0) && state.polygon_buffer_counts[0] == 1,
            "copied state survives callback changes and failure");
    if (failure == 1)
        require(state.scale == 12 && state.aspect == 13 && !services.focused,
                "callback failure leaves the later viewport stores untouched");
    else if (failure == 2)
        require(state.scale == 1 && state.aspect == 13 && !services.focused,
                "divide failure retains the scale store without an aspect or focus result");
    else
        require(services.focused && state.aspect == 0.5625f,
                "viewport comes from the last initialization callback, not an eager snapshot");
}
} // namespace
int main() {
    for (const auto precision : {0U, 512U, 768U})
        for (const auto rounding : {0U, 1024U, 2048U, 3072U}) {
            const auto control = static_cast<std::uint16_t>(precision | rounding | 127U);
            check(control, 0);
            check(control, 1);
            check(static_cast<std::uint16_t>(control & ~4U), 2);
        }
    std::cout << "Native library initialization: live callback owners, captured text identities "
                 "and callback/arithmetic store prefixes across 12 controls.\n";
}
