#include "guest/game_reset_reference.hpp"
#include "game/game_cleanup_internal.hpp"

namespace sadx {
namespace {
struct Stop {};
struct ResetAccess {
    GameResetState &state;
    GameResetResult &result;
    void clear_frame_message_work() { state.frame_message = 0; }
    void clear_editor_status() { state.editor = 0; }
    void *buffer(const std::optional<std::uint32_t> &identity, std::uint32_t site) {
        if (!identity) {
            result.failure = GameResetFailure::unknown_state;
            result.source_boundary = site;
            throw Stop{};
        }
        return state.tokens.host(*identity);
    }
    void *temporary_buffer() { return buffer(state.heap_at_3b290d0, 0x40bfb9); }
    void *secondary_buffer() { return buffer(state.heap_at_3b0e330, 0x40bfd1); }
    void clear_temporary_buffer() { state.heap_at_3b290d0 = 0; }
    void clear_secondary_buffer() { state.heap_at_3b0e330 = 0; }
    void seed_random(std::uint32_t seed) { crt_srand_6443b5(state.rng, seed); }
};
struct ResetOperations {
    GameResetRuntime &runtime;
    GameResetResult &result;
    void call(std::uint32_t site, std::uint32_t callee,
              std::optional<std::uint32_t> argument = {}) {
        result.pending_call = GameResetCall{site, callee, argument};
        result.source_boundary = site;
        if (!runtime.invoke) {
            result.failure = GameResetFailure::missing_owner;
            throw Stop{};
        }
        const auto receipt = runtime.invoke(*result.pending_call);
        result.nested_source_boundary = receipt.source_boundary;
        if (receipt.state != GameLogicOwnerState::completed) {
            result.failure = receipt.state == GameLogicOwnerState::cancelled
                ? GameResetFailure::cancelled : receipt.state == GameLogicOwnerState::failed
                ? GameResetFailure::owner_failed : GameResetFailure::missing_owner;
            throw Stop{};
        }
        result.pending_call.reset();
        result.source_boundary = 0;
        result.nested_source_boundary = 0;
    }
    void resume_voice_volume() { call(0x40bf42, 0x40cd10); }
    void set_depth_compare_mode(std::uint32_t mode) { call(0x40bf49, 0x401410, mode); }
    void stop_vibration(std::uint32_t port) { call(0x40bf51, 0x785330, port); }
    void set_blank_queue_mode() { call(0x40bf63, 0x403e60); }
    void reset_song_state() { call(0x40bf68, 0x4257c0); }
    void reset_current_voice() { call(0x40bf6d, 0x4257e0); }
    void stop_all_sounds() { call(0x40bf72, 0x424460); }
    void stop_pcm(std::int32_t channel) { call(0x40bf79, 0x410100, channel); }
    void close_media(std::uint32_t mode) { call(mode == 0 ? 0x40bf7f : 0x40bf86, 0x40cff0, mode); }
    void setup_screen() { call(0x40bf8b, 0x4208e0); }
    void display_message(void *) { call(0x40bf91, 0x40d490, 0); }
    void initialize_heap() { call(0x40bf99, 0x40b1a0); }
    void initialize_tasks() { call(0x40bf9e, 0x40b460); }
    void run_level_destructor(std::uint32_t mode) { call(0x40bfa6, 0x4228f0, mode); }
    void reset_loop_state() { call(0x40bfb4, 0x421fb0); }
    void release_buffer(void *token) {
        // Which slot is reached is observed on its lazy load, including a null first slot.
        const auto site = releasing_second ? 0x40bfdbU : 0x40bfc3U;
        call(site, 0x780fb0, runtime.state.tokens.guest(token));
    }
    bool releasing_second = false;
    void enable_fog() { call(0x40c00e, 0x411af0); }
};
// NOTSA: pair the source release-site observation with the shared lazy buffer read.
struct ObservedResetAccess : ResetAccess {
    ResetOperations &operations;
    void *temporary_buffer() {
        operations.releasing_second = false;
        return ResetAccess::temporary_buffer();
    }
    void *secondary_buffer() {
        operations.releasing_second = true;
        return ResetAccess::secondary_buffer();
    }
};
}
GameResetResult run_game_reset_40bf40(GameResetRuntime &runtime) {
    GameResetResult result;
    ResetOperations operations{runtime, result};
    ObservedResetAccess access{{runtime.state, result}, operations};
    try {
        cleanup::detail::reset(access, operations);
        result.completed = true;
    } catch (const Stop &) {}
    return result;
}
} // namespace sadx
