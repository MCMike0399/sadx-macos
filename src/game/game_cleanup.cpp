#include "game/game_cleanup.hpp"
#include <algorithm>
#include <stdexcept>
namespace sadx::cleanup {
void resume_voice_volume(std::int32_t &current, const std::int32_t &saved) { current = saved; }
std::int32_t stop_all_sounds(AudioQueueView &state,
                             const std::function<void(std::int32_t)> &stop_pcm) {
    for (std::size_t index = 0; index < state.records.size(); ++index) {
        state.records[index].timer = 0;
        state.records[index].flags |= 16;
        stop_pcm(static_cast<std::int32_t>(index));
        state.positional_tasks[index] = {};
    }
    return 0;
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
        std::copy(stages.begin(), stages.end(), state.completed_stages.begin() + character * 43);
    }
    state.missions = stored.missions;
    state.rumble_enabled = stored.vibration;
    state.market_rings = stored.total_rings;
}
void mission_level_destructor(void *) {
    throw std::runtime_error("LevelDestructor_MissionMode is not recovered");
}
void release_set_files(LevelState &state, const std::function<void(void *)> &release_buffer) {
    // The source debug formatter receives this constant, with no conversions.
    state.debug_message = "ReleaseSetFile\n";
    state.set_entry_count = nullptr;
    state.set_entries = nullptr;
    for (auto &file : state.set_files) {
        if (file != nullptr) {
            release_buffer(file);
            file = nullptr;
        }
    }
}
void release_camera_files(LevelState &state, const std::function<void(void *)> &release_buffer) {
    state.debug_message = "ReleaseCamFile\n";
    for (auto &file : state.camera_files) {
        if (file != nullptr) {
            release_buffer(file);
            file = nullptr;
        }
    }
}
void run_level_destructor(std::uint32_t mode, LevelState &state,
                           const std::function<void(void *)> &release_buffer) {
    if (mode == 0) {
        release_set_files(state, release_buffer);
        release_camera_files(state, release_buffer);
    } else if (mode == 5 && state.destructor == mission_level_destructor) {
        return;
    }
    const auto destructor = state.destructor;
    if (destructor != nullptr)
        destructor(state.destructor_context);
    state.destructor = nullptr;
    state.destructor_context = nullptr;
}
void reset_loop_state(LoopState &state, const std::function<void()> &clear_list) {
    const auto list = state.list_data;
    state.reset_work = 0;
    state.loop_count = 0;
    if (list != nullptr)
        clear_list();
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
        state.count = 0;
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
    services.resume_voice_volume();
    services.set_depth_compare_mode(1);
    services.stop_vibration(0);
    state.reset_status = 0;
    state.editor_status = 0;
    services.set_blank_queue_mode();
    services.reset_song_state();
    services.reset_current_voice();
    services.stop_all_sounds();
    services.stop_pcm(-1);
    services.close_media(0);
    services.close_media(1);
    services.setup_screen();
    services.display_message(nullptr);
    services.initialize_heap();
    services.initialize_tasks();
    for (std::uint32_t level = 0; level < 10; ++level)
        services.run_level_destructor(level);
    services.reset_loop_state();
    if (state.temporary_buffer != nullptr) {
        services.release_buffer(state.temporary_buffer);
        state.temporary_buffer = nullptr;
    }
    if (state.secondary_buffer != nullptr) {
        services.release_buffer(state.secondary_buffer);
        state.secondary_buffer = nullptr;
    }
    services.enable_fog();
    state.random_seed = 3735882923U;
}
} // namespace sadx::cleanup
