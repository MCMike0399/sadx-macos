#include "game/game_cleanup.hpp"
#include "game/game_cleanup_internal.hpp"
#include <algorithm>
#include <stdexcept>
namespace sadx::cleanup {
void resume_voice_volume(std::int32_t &current, const std::int32_t &saved) {
    struct Access {
        std::int32_t &current;
        const std::int32_t &saved;
        std::int32_t saved_voice_volume() { return saved; }
        void store_voice_volume(std::int32_t volume) { current = volume; }
    } access{current, saved};
    detail::resume_voice_volume(access);
}
std::int32_t stop_all_sounds(AudioQueueView &state,
                             const std::function<void(std::int32_t)> &stop_pcm) {
    struct Access {
        AudioQueueView &state;
        std::uint32_t sound_flags(std::size_t i) { return state.records[i].flags; }
        void clear_sound_timer(std::size_t i) { state.records[i].timer = 0; }
        void store_sound_flags(std::size_t i, std::uint32_t flags) { state.records[i].flags = flags; }
        void clear_positional_task(std::size_t i) { state.positional_tasks[i] = {}; }
    } access{state};
    return detail::stop_all_sounds(access, stop_pcm);
}

void set_player_number(std::uint16_t &current, std::uint16_t number) { current = number; }
std::uint16_t get_player_number(const std::uint16_t &current) { return current; }
void load_sequence_variables(const save::Data &save, std::array<std::uint8_t, 512> &flags) {
    for (std::size_t byte = 0; byte < save.sequence_variables.size(); ++byte) {
        for (unsigned bit = 0; bit < 8; ++bit)
            flags[byte * 8 + bit] = (save.sequence_variables[byte] >> bit) & 1;
    }
}
void load_cutscene_flags(const save::Data &save, std::array<std::uint8_t, 512> &flags) {
    for (std::size_t byte = 0; byte < save.event_flags.size(); ++byte) {
        for (unsigned bit = 0; bit < 8; ++bit)
            flags[byte * 8 + bit] = (save.event_flags[byte] >> bit) & 1;
    }
}
void load_playable_scenes(const save::Data &save, std::uint8_t *flags) {
    std::copy(save.playable_scene_flags.begin(), save.playable_scene_flags.end(), flags);
}
void load_story_sequences(const save::Data &save, std::array<bootstrap::StorySequence, 8> &state) {
    for (std::size_t index = 0; index < state.size(); ++index) {
        const auto &stored = save.sequences[index];
        auto &sequence = state[index];
        sequence.time = stored.time;
        // The source skips byte1: persisted padding is not the runtime emerald field.
        sequence.number = stored.number;
        sequence.section = stored.section;
        sequence.next_section = stored.next_section;
        sequence.stage = stored.stage;
        sequence.destination = stored.destination;
    }
}
void load_save(const save::Data &save, LoadSaveState &state) {
    const auto stored = save;
    state.global_timer = stored.total_time;
    std::uint8_t lives = stored.lives[6];
    if (state.playing_metal_sonic == 0) {
        const auto character = stored.last_character > 7 ? 0 : stored.last_character;
        set_player_number(state.character, character);
        // Save slots omit the two non-playable story characters.
        constexpr std::array<std::size_t, 8> life_slots{0, 0, 1, 2, 0, 3, 4, 5};
        lives = stored.lives[life_slots[get_player_number(state.character)]];
    }
    state.lives = static_cast<std::int8_t>(lives);
    state.saved_stage = stored.last_stage;
    load_sequence_variables(stored, state.sequence_variables);
    load_cutscene_flags(stored, state.cutscene_flags);
    load_playable_scenes(stored, state.playable_scene_flags);
    load_story_sequences(stored, state.sequences);
    for (std::size_t character = 0; character < stored.completed_stages.size(); ++character) {
        const auto &stages = stored.completed_stages[character];
        std::copy(stages.begin(), stages.end(),
                  state.completed_stages.begin() + character * save::kStagesPerCharacter);
    }
    state.missions = stored.missions;
    state.rumble_enabled = stored.vibration;
    state.market_rings = stored.total_rings;
}
void mission_level_destructor(void *) {
    throw std::runtime_error("LevelDestructor_MissionMode is not recovered");
}
namespace {
// NOTSA: plain native access to the borrowed level owners.
struct LevelAccess {
    LevelState &state;
    void print_debug(const char *text) { detail::format_literal_debug(text, state.debug_message); }
    void clear_set_count() { state.set_entry_count = nullptr; }
    void clear_set_entries() { state.set_entries = nullptr; }
    void *set_file(std::size_t i) { return state.set_files[i]; }
    void *camera_file(std::size_t i) { return state.camera_files[i]; }
    void clear_set_file(std::size_t i) { state.set_files[i] = nullptr; }
    void clear_camera_file(std::size_t i) { state.camera_files[i] = nullptr; }
    bool is_mission_destructor() { return state.destructor == mission_level_destructor; }
    void invoke_level_destructor() {
        if (const auto callback = state.destructor)
            callback(state.destructor_context);
    }
    void clear_level_destructor() {
        state.destructor = nullptr;
        state.destructor_context = nullptr;
    }
};
}
void release_set_files(LevelState &state, const std::function<void(void *)> &release_buffer) {
    LevelAccess access{state};
    detail::release_set_files(access, release_buffer);
}
void release_camera_files(LevelState &state, const std::function<void(void *)> &release_buffer) {
    LevelAccess access{state};
    detail::release_camera_files(access, release_buffer);
}
void run_level_destructor(std::uint32_t mode, LevelState &state,
                          const std::function<void(void *)> &release_buffer) {
    LevelAccess access{state};
    detail::run_level_destructor(mode, access, release_buffer);
}
void reset_loop_state(LoopState &state, const std::function<void()> &clear_list) {
    struct Access {
        LoopState &state;
        bool list_present() { return state.list_data != nullptr; }
        void clear_reset_work() { state.reset_work = 0; }
        void clear_loop_count() { state.loop_count = 0; }
    } access{state};
    detail::reset_loop_state(access, clear_list);
}
void release_regular_object_textures(TextureState &state, const ReleaseTextureList &release) {
    if (state.common != nullptr) {
        release(*state.common);
        state.common = nullptr;
    }
    for (auto &list : state.regular) {
        if (list == nullptr)
            break;
        release(*list);
        list = nullptr;
    }
    // A full source table ends at the adjacent, already-cleared common pointer.
}
void release_character_textures(TextureState &state, const ReleaseTextureList &release) {
    for (std::size_t character = 0; character < state.characters.size(); ++character) {
        const auto *entry = state.characters[character];
        if (entry == nullptr)
            continue;
        if (state.loaded[character] > 0) {
            for (auto *list : entry->lists) {
                if (list == nullptr)
                    break;
                release(*list);
            }
        }
        state.loaded[character] = 0;
    }
}
void unload_character_textures(std::size_t character, TextureState &state,
                               const ReleaseTextureList &release) {
    const auto *entry = state.characters.at(character);
    if (entry == nullptr)
        return;
    if (state.loaded[character] == 1) {
        for (auto *list : entry->lists) {
            if (list == nullptr)
                break;
            if (list->entries.empty())
                throw std::runtime_error("UnloadCharTextures requires the declared first texture name");
            if (list->entries.front() != nullptr)
                release(*list);
        }
    }
    if (state.loaded[character] > 0)
        --state.loaded[character];
}
void clear(ClearServices &services) {
    services.release_regular_object_textures();
    services.release_character_textures();
    services.clear_texture_binding();
}
std::int32_t reset_auxiliary_work(AuxiliaryWork &state) {
    state.values.fill(0);
    state.limit = 50;
    return 1;
}
void clear_sequence_running(std::int32_t &running) { running = 0; }
void reset_song_state(TrackState &state) {
    state.current_song = -1;
    state.song_queue_state = 0;
    state.previous_song = -1;
    state.real_previous_song = -1;
}
void reset_current_voice(TrackState &state) {
    state.current_voice = -1;
    state.voice_queue_state = 0;
}
void set_stage_number(std::uint8_t stage, std::uint8_t act, StageState &state) {
    state.previous_stage = state.stage;
    state.previous_act = (state.previous_act & 4294901760U) |
                         static_cast<std::uint16_t>(state.act);
    state.stage = stage;
    state.act = act;
}
void display_message(const Message *message, MessageState &state,
                      const std::uint8_t &control_message_gate,
                      const std::int32_t &pending_logic_ticks) {
    if (message == nullptr) {
        struct Access {
            MessageState &state;
            void clear_message_count() { state.count = 0; }
        } access{state};
        detail::clear_messages(access);
        return;
    }
    const auto count = static_cast<std::int32_t>(state.count);
    if (count < 4 && (message->kind != 1 || control_message_gate != 0) && pending_logic_ticks == 0) {
        if (count < 0)
            throw std::out_of_range("MSG_Disp indexes before its four message slots");
        state.entries[static_cast<std::size_t>(count)] = message;
        ++state.count;
    }
}
void prepare_title(TitleState &state, TitleServices &services) {
    services.initialize_heap();
    services.initialize_tasks();
    state.demo_mode = 0;
    state.demo_event = 0;
    state.demo_reset = 0;
    state.demo_enabled = 0;
    state.demo_paused = 0;
    reset_auxiliary_work(state.auxiliary);
    state.hide_hud = 0;
    state.pause_state = 0;
    clear_sequence_running(state.sequence_running);
    state.story_event_mode = 0;
    state.backup = state.save;
    services.load_save();
    services.set_fade_color(0);
    services.set_stage_number(0, 0);
}

void reset(State &state, Services &services) {
    struct Access {
        State &state;
        void clear_frame_message_work() { state.frame_message_work = 0; }
        void clear_editor_status() { state.editor_status = 0; }
        void *temporary_buffer() { return state.temporary_buffer; }
        void *secondary_buffer() { return state.secondary_buffer; }
        void clear_temporary_buffer() { state.temporary_buffer = nullptr; }
        void clear_secondary_buffer() { state.secondary_buffer = nullptr; }
        void seed_random(std::uint32_t seed) { state.random_seed = seed; }
    } access{state};
    detail::reset(access, services);
}
} // namespace sadx::cleanup
