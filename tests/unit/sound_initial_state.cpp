#include "game/sound_initial_state.hpp"
#include "game/sound_manager.hpp"
#include <iostream>
#include <stdexcept>

int main() {
    auto values = sadx::sound_initial_state();
    sadx::AudioStartupState audio{};
    audio.word_at_3b29ce0 = values.paused;
    audio.word_at_3b29ce4 = values.positional_paused;
    audio.byte_at_3b29ce8 = values.frame_counter;
    std::array<std::optional<std::uint32_t>, 36> flags{}, tasks{};
    sadx::SoundRecordFields fields{};
    // Explicit null resource facts for this cold, idle sound server fixture.
    fields.listener_at_3b2cbb0 = 0;
    for (unsigned i = 0; i < values.records.size(); ++i) {
        audio.record_words_at_3b292fc[i] = static_cast<std::uint32_t>(values.records[i].timer);
        flags[i] = values.records[i].flags;
        fields.ids_at_3b29308[i] = static_cast<std::uint32_t>(values.records[i].tone);
    }
    unsigned calls = 0;
    sadx::SoundManagerRuntime runtime{
        audio, flags, tasks, fields, [&](const sadx::SoundManagerCall &) {
            ++calls;
            return sadx::SoundManagerOwnerResult{{sadx::GameLogicOwnerState::failed, 0, {}}, {}};
        }};
    const auto result = sadx::run_sound_manager_4250d0(runtime);
    if (!result || calls || audio.byte_at_3b29ce8 != 1)
        throw std::runtime_error("cold sound defaults run the idle server without helper calls");
    for (unsigned i = 0; i < values.records.size(); ++i)
        if (values.records[i].tone != 0 || fields.ids_at_3b29308[i] != 0xffffffffU ||
            fields.words_at_3b29300[i] != 0 || tasks[i] != 0)
            throw std::runtime_error("idle server clears tones after load-time initialization");
    values.overlap[71] = 1;
    values.records[35].tone = 7;
    if (sadx::sound_initial_state().overlap[71] || sadx::sound_initial_state().records[35].tone)
        throw std::runtime_error("sound sessions own independent initial values");
    std::cout
        << "Sound defaults: idle server, post-service tone and independent sessions passed.\n";
}
