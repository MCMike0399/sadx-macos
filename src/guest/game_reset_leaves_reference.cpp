#include "guest/game_reset_leaves_reference.hpp"
#include "game/game_cleanup_internal.hpp"

namespace sadx {
namespace {
struct Stop {
    ResetLeafOutcome outcome;
    std::uint32_t site;
    GameLogicOwnerResult nested;
};
struct LeafAccess {
    ResetLeafState &state;
    const ResetLeafNested &nested;
    template<class T> T read(const std::optional<T> &value, std::uint32_t site) {
        if (!value) throw Stop{ResetLeafOutcome::unknown_state, site, {}};
        return *value;
    }
    void call(std::uint32_t site, std::uint32_t callee,
              std::optional<std::uint32_t> object = {}, std::optional<std::uint32_t> argument = {}) {
        const auto result = nested ? nested({site, callee, object, argument})
                                  : GameLogicOwnerResult{GameLogicOwnerState::unavailable, callee, {}};
        if (result.state != GameLogicOwnerState::completed)
            throw Stop{ResetLeafOutcome::nested_failed, site, result};
    }
    void stop_pcm(std::int32_t) {
        if (read(state.audio_initialized_at_3b0ef2c, 0x410100) != 0)
            throw Stop{ResetLeafOutcome::nested_failed, 0x41010d,
                       {GameLogicOwnerState::unavailable, 0x41010d, {}}};
    }
    std::uint32_t saved_voice_volume() { return read(state.word_at_3abdf90, 0x40cd10); }
    void store_voice_volume(std::uint32_t value) { state.word_at_3abdfac = value; }
    void store_depth_compare_mode(std::uint32_t mode) { state.depth_mode_at_8928c8 = mode; }
    void store_queue_mode(std::uint32_t mode) { state.queue_mode_at_3ab98ac = mode; }
    bool list_present() { return read(state.word_at_3c5e8b8, 0x421fb0) != 0; }
    void clear_reset_work() { state.word_at_3b11190 = 0; }
    void clear_loop_count() { state.timing_word_at_3b1118c = 0; }
    void clear_list() { call(0x421fc6, 0x504ed0); }
    std::uint32_t sound_flags(std::size_t i) { return read(state.sound_record_flags_at_3b29304[i], 0x424470); }
    void clear_sound_timer(std::size_t i) { state.sound_record_words_at_3b292fc[i] = 0; }
    void store_sound_flags(std::size_t i, std::uint32_t flags) { state.sound_record_flags_at_3b29304[i] = flags; }
    void clear_positional_task(std::size_t i) { state.words_at_3b29b90[i] = 0; }
    bool music_open() { return read(state.word_at_3abdfa0, 0x40d020) != 0; }
    bool voice_present() { return read(state.player_at_3abdf94, 0x40cffe) != 0; }
    void stop_music() { call(0x40d02e, 0x411760, read(state.player_at_3abdf9c, 0x40d028)); }
    void reset_music() { call(0x40d039, 0x411720, read(state.player_at_3abdf9c, 0x40d033)); }
    void stop_voice() { call(0x40d008, 0x411760, read(state.player_at_3abdf94, 0x40cffe)); }
    void reset_voice() { call(0x40d013, 0x411720, read(state.player_at_3abdf94, 0x40d00d)); }
    void clear_music_open() { state.word_at_3abdfa0 = 0; }
    void clear_music_status() { state.word_at_3abdf98 = 0; }
    void clear_shared_status() { state.word_at_3abdfa8 = 0; }
    void clear_music_looping() { state.word_at_3abdfa4 = 0; }
    bool audio_ready() { return read(state.audio_initialized_at_3b0ef2c, 0x40ff10) != 0; }
    bool music_enabled() { return read(state.config_at_3a7cf3c, 0x40d0f3) != 0; }
    bool voice_enabled() { return read(state.config_at_38f6ec0, 0x40d175) != 0; }
    void create_music() {
        call(0x40d0fd, 0x6443e7, {}, 0x50);
        state.player_at_3abdf9c.reset();
    }
    void create_voice() {
        call(0x40d17f, 0x6443e7, {}, 0x50);
        state.player_at_3abdf94.reset();
    }
    void clear_music_player() { state.player_at_3abdf9c = 0; }
    void clear_voice_player() { state.player_at_3abdf94 = 0; }
    void print_debug(const char *text) {
        cleanup::detail::format_literal_debug(text, state.debug_text_at_38f6d08);
    }
    void clear_set_count() { state.word_at_3c4e458 = 0; }
    void clear_set_entries() { state.word_at_3c4e45c = 0; }
    void *set_file(std::size_t i) { return state.tokens.host(read(state.set_files_at_3abdf40[i], 0x422460)); }
    void *camera_file(std::size_t i) { return state.tokens.host(read(state.cam_files_at_3abdf28[i], 0x4224c0)); }
    void clear_set_file(std::size_t i) { state.set_files_at_3abdf40[i] = 0; }
    void clear_camera_file(std::size_t i) { state.cam_files_at_3abdf28[i] = 0; }
    bool is_mission_destructor() { return read(state.callback_at_3b291b4, 0x422909) == 0x592370U; }
    void invoke_level_destructor() {
        const auto callback = read(state.callback_at_3b291b4, 0x422915);
        if (callback != 0) call(0x42291e, callback);
    }
    void clear_level_destructor() { state.callback_at_3b291b4 = 0; }
    void release_set(void *token) { call(0x42246b, 0x780fb0, {}, state.tokens.guest(token)); }
    void release_camera(void *token) { call(0x4224cb, 0x780fb0, {}, state.tokens.guest(token)); }
    void clear_message_count() { state.word_at_3b0e338 = 0; }
    void observe_vibration_comparison() { (void)read(state.word_at_389d608, 0x785334); }
    std::uint32_t first_vibration_entry() { return read(state.entry_words_at_3d08428[0], 0x78534f); }
    void store_first_vibration_entry(std::uint32_t value) { state.entry_words_at_3d08428[0] = value; }
};
// NOTSA: release call-site mapping is reference metadata; the loop is shared.
struct LevelAccess : LeafAccess {
    bool camera = false;
    void print_debug(const char *text) {
        camera = std::string(text) == "ReleaseCamFile\n";
        LeafAccess::print_debug(text);
    }
    void release(void *token) { camera ? release_camera(token) : release_set(token); }
};
}
bool reset_leaf_print_debug_401000(const std::string &format, ResetLeafState &state) {
    return cleanup::detail::format_literal_debug(format, state.debug_text_at_38f6d08);
}
ResetLeafResult run_reset_leaf(std::uint32_t callee, std::optional<std::uint32_t> argument,
                               ResetLeafState &state, const ResetLeafNested &nested) {
    ResetLeafResult result;
    result.outcome = ResetLeafOutcome::completed;
    LevelAccess access{{state, nested}};
    try {
        // Routing and ABI validation only. Recovered algorithms live in cleanup.
        switch (callee) {
        case 0x40cd10: cleanup::detail::resume_voice_volume(access); break;
        case 0x401410:
            cleanup::detail::set_depth_compare_mode(access.read(argument, 0x401410), access); break;
        case 0x403e60: cleanup::detail::set_blank_queue_mode(access); break;
        case 0x421fb0:
            cleanup::detail::reset_loop_state(access, [&] { access.clear_list(); }); break;
        case 0x4228f0:
            cleanup::detail::run_level_destructor(access.read(argument, 0x4228f0), access,
                                                  [&](void *token) { access.release(token); }); break;
        case 0x410100: access.stop_pcm(0); break;
        case 0x424460:
            result.eax = cleanup::detail::stop_all_sounds(access, [&](auto channel) { access.stop_pcm(channel); });
            break;
        case 0x40cff0: cleanup::detail::close_media(access.read(argument, 0x40cff0), access); break;
        case 0x422440:
            cleanup::detail::release_set_files(access, [&](void *token) { access.release_set(token); }); break;
        case 0x4224b0:
            cleanup::detail::release_camera_files(access, [&](void *token) { access.release_camera(token); }); break;
        case 0x4253c0: case 0x40d0d0: cleanup::detail::initialize_media(access); break;
        case 0x40d490:
            if (access.read(argument, 0x40d490) != 0)
                throw Stop{ResetLeafOutcome::nested_failed, 0x40d49f,
                           {GameLogicOwnerState::unavailable, 0x40d49f, {}}};
            cleanup::detail::clear_messages(access); break;
        case 0x785330:
            result.eax = cleanup::detail::stop_original_vibration(access.read(argument, 0x785330), access); break;
        default: result.outcome = ResetLeafOutcome::not_a_leaf; break;
        }
    } catch (const Stop &stop) {
        result.outcome = stop.outcome;
        result.source_boundary = stop.site;
        result.nested = stop.nested;
    }
    return result;
}
} // namespace sadx
