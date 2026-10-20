#include "guest/game_library_init_reference.hpp"
#include "game/game_library_init_internal.hpp"
#include <cstring>

namespace sadx {
namespace {
struct Stop {};
// NOTSA: source availability, opaque resource words and call-site diagnostics only.
struct ReferenceContext final : library::Context {
    explicit ReferenceContext(GameLibraryInitRuntime &runtime)
        : Context(runtime.fp), runtime(runtime) {}
    GameLibraryInitRuntime &runtime;
    GameLibraryInitResult result;
    std::array<std::uint32_t, 4> captured_paths{};
    [[noreturn]] void fail(GameLibraryInitFailure failure, std::uint32_t site,
                          std::optional<std::uint32_t> fact = {}) {
        result.failure = failure;
        result.source_boundary = site;
        result.required_fact = fact;
        throw Stop{};
    }
    std::uint32_t configuration_word(std::uint32_t offset, std::uint32_t site) {
        const auto address = runtime.config + offset;
        const auto value = runtime.read ? runtime.read(address) : std::nullopt;
        if (!value)
            fail(GameLibraryInitFailure::unknown_state, site, address);
        return *value;
    }
    std::optional<std::uint32_t>
    invoke(std::uint32_t site, std::uint32_t callee,
           std::array<std::optional<std::uint32_t>, 4> arguments = {}) {
        result.pending_call = GameLibraryCall{site, callee, arguments};
        if (!runtime.invoke)
            fail(GameLibraryInitFailure::missing_owner, site);
        const auto receipt = runtime.invoke(*result.pending_call);
        if (receipt.owner.state != GameLogicOwnerState::completed) {
            const auto failure = receipt.owner.state == GameLogicOwnerState::cancelled
                                     ? GameLibraryInitFailure::cancelled
                                 : receipt.owner.state == GameLogicOwnerState::failed
                                     ? GameLibraryInitFailure::owner_failed
                                     : GameLibraryInitFailure::missing_owner;
            fail(failure, site);
        }
        result.pending_call.reset();
        return receipt.eax;
    }
    void capture_application_paths() override {
        const auto application = configuration_word(4, 0x78107a);
        const auto company = configuration_word(0, 0x78107d);
        const auto system = configuration_word(8, 0x78107f);
        const auto backup = configuration_word(12, 0x781086);
        captured_paths = {company, application, system, backup};
    }
    std::uint32_t configured_polygon_buffer_count(std::size_t index) override {
        constexpr std::array<std::uint32_t, 9> sites{
            0x78108d, 0x78109a, 0x7810a7, 0x7810af, 0x7810b8,
            0x7810c1, 0x7810c9, 0x7810d2, 0x7810db};
        return configuration_word(0x28 + 4 * static_cast<std::uint32_t>(index), sites.at(index));
    }
    void set_polygon_buffer_count(std::size_t index, std::uint32_t value) override {
        runtime.state.words_at_389d864.at(index) = value;
    }
    std::uint32_t configured_windowed() override { return configuration_word(0x10, 0x7810e3); }
    void set_windowed(std::uint32_t value) override { runtime.state.word_at_38a5dc4 = value; }
    std::uint32_t configured_back_buffer_width() override { return configuration_word(0x50, 0x7810ec); }
    void set_back_buffer_width(std::uint32_t value) override { runtime.state.word_at_38a5dc8 = value; }
    std::uint32_t configured_back_buffer_height() override { return configuration_word(0x54, 0x7810f5); }
    void set_back_buffer_height(std::uint32_t value) override { runtime.state.word_at_38a5dcc = value; }
    std::uint32_t configured_high_precision_depth_request() override { return configuration_word(0x4c, 0x7810fd); }
    std::uint32_t configured_screen_flags() override { return configuration_word(0x14, 0x781139); }
    std::uint32_t configured_screen_setup_argument() override { return configuration_word(0x18, 0x781136); }
    std::uint32_t configured_frame_interval() override { return configuration_word(0x1c, 0x781133); }
    std::uint32_t viewport_word(std::uint32_t address, std::uint32_t site) {
        const auto value = runtime.read_viewport ? runtime.read_viewport(address) : std::nullopt;
        if (!value)
            fail(GameLibraryInitFailure::unknown_state, site, address);
        return *value;
    }
    std::uint32_t viewport_height() override { return viewport_word(0x3d1278c, 0x7811fd); }
    std::uint32_t viewport_width() override { return viewport_word(0x3d12788, 0x78121d); }
    void scale(float value) override { runtime.state.scale_at_3d0f800 = value; }
    void aspect(float value) override { runtime.state.aspect_at_3d0f9c0 = value; }
    void configure_application_paths() override {
        (void)invoke(0x78110d, 0x793ef0,
                     {captured_paths[0], captured_paths[1], captured_paths[2], captured_paths[3]});
    }
    void set_library_mode(std::uint32_t mode) override {
        (void)invoke(0x781120, 0x77ff40, {mode});
    }
    void configure_screen(std::uint32_t flags, std::uint32_t setup_argument,
                          std::uint32_t interval) override {
        (void)invoke(0x78113f, 0x780c30, {flags, setup_argument, interval});
    }
    std::int32_t initialize_file_system(std::uint32_t handles, std::uint32_t entries) override {
        const auto returned = invoke(0x781189, 0x788000, {handles, 0x3ce71c0U, entries, 0x3cf4580U});
        if (!returned)
            fail(GameLibraryInitFailure::owner_failed, 0x781191);
        std::int32_t signed_result;
        std::memcpy(&signed_result, &*returned, sizeof(signed_result));
        return signed_result;
    }
    void enable_high_precision_depth() override { (void)invoke(0x781103, 0x794050); }
    void initialize_window() override { (void)invoke(0x781112, 0x789f80); }
    void initialize_graphics() override { (void)invoke(0x781125, 0x793d80); }
    void create_heap() override { (void)invoke(0x78112e, 0x780f20, {0U, 0U}); }
    void initialize_input() override { (void)invoke(0x781155, 0x77f150, {0U, 0x3cfc400U, 0x3ce8550U}); }
    void register_debug_text() override { (void)invoke(0x78115d, 0x793e70); }
    void focus_window() override { (void)invoke(0x78123a, 0x789c50); }
    std::uint32_t cleanup_site(std::uint32_t ordinal) const {
        return (cleanup_reason == library::CleanupReason::Retry ? 0x7811a1U : 0x7811d5U) +
               5 * ordinal;
    }
    void shutdown_file_system() override { (void)invoke(cleanup_site(0), 0x7881a0); }
    void unregister_debug_text() override { (void)invoke(cleanup_site(1), 0x793e80); }
    void shutdown_input() override { (void)invoke(cleanup_site(2), 0x77f220); }
    void release_system_pool() override { (void)invoke(cleanup_site(3), 0x780e70); }
    void destroy_heap() override { (void)invoke(cleanup_site(4), 0x780f40); }
    void release_graphics() override { (void)invoke(cleanup_site(5), 0x793d90); }
    void close_instance_mutex() override { (void)invoke(cleanup_site(6), 0x789d30); }
};
} // namespace
GameLibraryInitResult run_library_init_781070(GameLibraryInitRuntime &runtime) {
    ReferenceContext context(runtime);
    try {
        (void)library::initialize_libraries(context);
    } catch (Stop) {
    } catch (const X87Error &) {
        context.result.failure = GameLibraryInitFailure::fp_failed;
        context.result.source_boundary = 0x781232;
    }
    context.result.tries_left = context.attempts_remaining;
    return context.result;
}
} // namespace sadx
