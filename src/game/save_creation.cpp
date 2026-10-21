#include "game/save_creation.hpp"
#include <algorithm>
#include <stdexcept>
namespace sadx::save {
namespace {
// NOTSA: source913b5c maps story characters onto the persisted
// playable-character life slots.
std::size_t life_slot(std::uint16_t character) {
    constexpr std::array<std::size_t, 8> slots{0, 0, 1, 2, 0, 3, 4, 5};
    if (character >= slots.size())
        throw std::out_of_range("save capture character exceeds source life-slot table");
    return slots[character];
}
// NOTSA: the source clears each packed byte then sets bits ONLY for flag
// value1.
void pack_flags(std::array<std::uint8_t, 64> &packed, const std::array<std::uint8_t, 512> &flags) {
    for (std::size_t byte = 0; byte < packed.size(); ++byte) {
        packed[byte] = 0;
        for (unsigned bit = 0; bit < 8; ++bit)
            if (flags[byte * 8 + bit] == 1)
                packed[byte] |= static_cast<std::uint8_t>(1U << bit);
    }
}
} // namespace
void prepare_new(bootstrap::State &story, Data &data, std::int16_t &rings) {
    bootstrap::initialize_story_flags(story);
    initialize(data, rings);
}
void restore_title_lives(const Data &data, CaptureState &state) {
    if (state.global_mode == 9) {
        auto index = static_cast<std::int32_t>(state.character);
        if (index > 4)
            --index;
        if (index > 1)
            --index;
        if (index < 0 || static_cast<std::size_t>(index) >= data.lives.size())
            throw std::out_of_range("title lives index exceeds persisted source slots");
        state.lives = static_cast<std::int8_t>(data.lives[static_cast<std::size_t>(index)]);
    }
}
void store_sequence_variables(Data &data, const std::array<std::uint8_t, 512> &flags) {
    pack_flags(data.sequence_variables, flags);
}
void store_cutscene_flags(Data &data, const std::array<std::uint8_t, 512> &flags) {
    pack_flags(data.event_flags, flags);
}
void store_playable_scenes(Data &data, const std::uint8_t *flags) {
    if (!flags)
        throw std::invalid_argument("save capture has no live playable-scene flags");
    std::copy_n(flags, data.playable_scene_flags.size(), data.playable_scene_flags.begin());
}
void store_story_sequences(Data &data, const std::array<bootstrap::StorySequence, 8> &sequences) {
    for (std::size_t index = 0; index < sequences.size(); ++index) {
        const auto &source = sequences[index];
        auto &destination = data.sequences[index];
        destination.time = source.time;
        // Byte1 is persisted padding; the source skips the runtime emerald field.
        destination.number = source.number;
        destination.section = source.section;
        destination.next_section = source.next_section;
        destination.stage = source.stage;
        destination.destination = source.destination;
    }
}
void capture_live_data(Data &data, CaptureState &state) {
    auto captured = data;
    captured.total_time = state.global_timer;
    if (state.playing_metal_sonic != 0)
        captured.lives[6] = static_cast<std::uint8_t>(state.lives);
    else {
        restore_title_lives(data, state);
        captured.lives[life_slot(state.character)] = static_cast<std::uint8_t>(state.lives);
    }
    auto character = static_cast<std::uint8_t>(state.character);
    if (character == 0) {
        if (state.playing_metal_sonic != 0)
            character = 8;
        else if (state.playing_super_sonic != 0)
            character = 9;
    }
    captured.last_character = character;
    captured.last_stage = static_cast<std::uint16_t>(state.saved_stage);
    store_sequence_variables(captured, state.sequence_variables);
    store_cutscene_flags(captured, state.cutscene_flags);
    store_playable_scenes(captured, state.playable_scene_flags);
    store_story_sequences(captured, state.sequences);
    for (std::size_t index = 0; index < captured.completed_stages.size(); ++index)
        std::copy_n(state.completed_stages.begin() + index * 43, 43,
                    captured.completed_stages[index].begin());
    captured.missions = state.missions;
    captured.total_rings = state.market_rings;
    captured.vibration = state.rumble_enabled;
    captured.checksum = checksum(encode(captured));
    data = captured;
}
std::int32_t request_write(const std::int32_t &saving_disabled, std::uint8_t &save_ready,
                           const std::function<void()> &capture_live) {
    if (saving_disabled == 0)
        save_ready = 1;
    else
        capture_live();
    return 0;
}
} // namespace sadx::save
