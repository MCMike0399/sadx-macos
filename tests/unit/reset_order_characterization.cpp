#include "guest/game_reset_reference.hpp"
#include "guest/game_reset_leaves_reference.hpp"
#include "game/game_cleanup.hpp"
#include "game/audio_playback.hpp"
#include <iostream>
#include <stdexcept>
#include <vector>

namespace {
enum class LeafScenario : unsigned {
    MissingFirstFlags,
    MissingReadiness,
    InitializedAudio,
    MissingSecondFlags,
    MissingListPresence,
    PresentList,
    ReplaceMusicPlayer,
    InvalidateMusicPlayer,
    ClearVoicePlayer,
    ReplaceLaterSetFileAndCallback,
    FailedRelease,
    MissingLevelCallback,
    MissionDestructor,
    MissingVibrationComparison,
};

struct Fixture {
    std::optional<std::uint32_t> depth = 8, queue = 9, ready = 0;
    std::optional<std::int32_t> timing = 7;
    std::array<std::optional<std::uint32_t>, 36> records{};
    sadx::ResetLeafState state{{}, {}, depth, queue, 6, 5, timing, 0, records, {}, {}, ready,
                              10, 11, 1, 12, 13, 14, 1, 1, 15, {}, 16, 17, 18, {}, {}, {}};
    Fixture() {
        records.fill(99);
        state.sound_record_flags_at_3b29304.fill(2);
        state.words_at_3b29b90.fill(3);
        state.set_files_at_3abdf40.fill(0);
        state.cam_files_at_3abdf28.fill(0);
        state.entry_words_at_3d08428.fill(0);
    }
};

void emit(const char *name, const sadx::ResetLeafResult &result, Fixture &fixture,
          const std::vector<sadx::ResetLeafCall> &calls) {
    std::cout << name << ' ' << static_cast<int>(result.outcome) << ' ' << result.source_boundary
              << ' ' << result.eax.value_or(123) << ' ' << static_cast<int>(result.nested.state)
              << ' ' << result.nested.source_boundary;
    const auto word = [](const auto &value) {
        std::cout << ' ' << (value ? std::to_string(*value) : "unknown");
    };
    for (const auto &value : fixture.records)
        word(value);
    for (const auto &value : fixture.state.sound_record_flags_at_3b29304)
        word(value);
    for (const auto &value : fixture.state.words_at_3b29b90)
        word(value);
    for (const auto &value : fixture.state.set_files_at_3abdf40)
        word(value);
    for (const auto &value : fixture.state.cam_files_at_3abdf28)
        word(value);
    word(fixture.state.word_at_3b11190);
    word(fixture.timing);
    word(fixture.state.callback_at_3b291b4);
    word(fixture.state.player_at_3abdf94);
    word(fixture.state.player_at_3abdf9c);
    word(fixture.state.word_at_3abdfa0);
    word(fixture.state.word_at_3abdf98);
    word(fixture.state.word_at_3abdfa8);
    word(fixture.state.word_at_3c4e458);
    word(fixture.state.word_at_3c4e45c);
    for (const auto &call : calls) {
        std::cout << " call:" << call.source_call << ':' << call.callee << ':'
                  << call.this_object.value_or(0) << ':' << call.argument.value_or(0);
    }
    std::cout << '\n';
}

void check_leaf_order() {
    for (unsigned case_id = 0; case_id < 14; ++case_id) {
        const auto scenario = static_cast<LeafScenario>(case_id);
        Fixture fixture;
        std::vector<sadx::ResetLeafCall> calls;
        auto callee = 0x424460U;
        std::optional<std::uint32_t> argument = 0;
        if (scenario == LeafScenario::MissingFirstFlags)
            fixture.state.sound_record_flags_at_3b29304[0].reset();
        if (scenario == LeafScenario::MissingReadiness)
            fixture.ready.reset();
        if (scenario == LeafScenario::InitializedAudio)
            fixture.ready = 1;
        if (scenario == LeafScenario::MissingSecondFlags)
            fixture.state.sound_record_flags_at_3b29304[1].reset();
        if (scenario == LeafScenario::MissingListPresence || scenario == LeafScenario::PresentList) {
            callee = 0x421fb0;
            if (scenario == LeafScenario::MissingListPresence)
                fixture.state.word_at_3c5e8b8.reset();
        }
        if (scenario == LeafScenario::ReplaceMusicPlayer ||
            scenario == LeafScenario::InvalidateMusicPlayer || scenario == LeafScenario::ClearVoicePlayer) {
            callee = 0x40cff0;
            argument = scenario == LeafScenario::ClearVoicePlayer ? 1 : 0;
        }
        if (scenario == LeafScenario::ReplaceLaterSetFileAndCallback ||
            scenario == LeafScenario::FailedRelease || scenario == LeafScenario::MissingLevelCallback ||
            scenario == LeafScenario::MissionDestructor) {
            callee = 0x4228f0;
            argument = scenario == LeafScenario::MissionDestructor ? 5 : 0;
            fixture.state.callback_at_3b291b4 =
                scenario == LeafScenario::MissionDestructor ? 0x592370U : 99;
            fixture.state.set_files_at_3abdf40[0] = 42;
            fixture.state.set_files_at_3abdf40[1] = 42;
            if (scenario == LeafScenario::MissingLevelCallback)
                fixture.state.callback_at_3b291b4.reset();
        }
        if (scenario == LeafScenario::MissingVibrationComparison) {
            callee = 0x785330;
            fixture.state.word_at_389d608.reset();
        }
        const auto result = sadx::run_reset_leaf(callee, argument, fixture.state, [&](const auto &call) {
            calls.push_back(call);
            if (scenario == LeafScenario::ReplaceMusicPlayer)
                fixture.state.player_at_3abdf9c = 27;
            if (scenario == LeafScenario::InvalidateMusicPlayer)
                fixture.state.player_at_3abdf9c.reset();
            if (scenario == LeafScenario::ClearVoicePlayer)
                fixture.state.player_at_3abdf94 = 0;
            if (scenario == LeafScenario::ReplaceLaterSetFileAndCallback) {
                fixture.state.set_files_at_3abdf40[1] = 43;
                fixture.state.callback_at_3b291b4 = 98;
            }
            return sadx::GameLogicOwnerResult{
                scenario == LeafScenario::FailedRelease ? sadx::GameLogicOwnerState::failed
                                                       : sadx::GameLogicOwnerState::completed,
                0, {}};
        });
        emit(std::to_string(case_id).c_str(), result, fixture, calls);
    }
}

// The numeric IDs remain the frozen output identities. Cases 10..39 fail at calls 1..30.
unsigned failed_call_for_case(unsigned case_id) {
    switch (case_id) {
    case 2: return 1;
    case 3: return 3;
    case 4: return 4;
    case 5: return 26;
    case 6: return 27;
    default: return case_id >= 10 ? case_id - 9 : 999;
    }
}

void check_reset_order() {
    constexpr unsigned missing_first_buffer = 0;
    constexpr unsigned replace_second_buffer = 1;
    constexpr unsigned missing_owner = 7;
    constexpr unsigned cancelled_owner = 8;
    constexpr unsigned unavailable_owner = 9;
    for (unsigned case_id = 0; case_id < 40; ++case_id) {
        sadx::reference::FrameMessageFact frame = 7;
        sadx::CrtRandState random;
        sadx::GameResetState state{
            frame, 65535, case_id == missing_first_buffer ? std::optional<std::uint32_t>{} : 42,
            42, random};
        std::vector<sadx::GameResetCall> calls;
        sadx::GameResetRuntime runtime{state, [&](const auto &call) {
            calls.push_back(call);
            if (call.callee == 0x780fb0 && case_id == replace_second_buffer)
                state.heap_at_3b0e330 = 43;
            return sadx::GameLogicOwnerResult{
                calls.size() == failed_call_for_case(case_id) ? sadx::GameLogicOwnerState::failed
                                                             : sadx::GameLogicOwnerState::completed,
                123, {}};
        }};
        if (case_id == missing_owner)
            runtime.invoke = {};
        if (case_id == cancelled_owner || case_id == unavailable_owner) {
            runtime.invoke = [&](const auto &call) {
                calls.push_back(call);
                return sadx::GameLogicOwnerResult{
                    case_id == cancelled_owner ? sadx::GameLogicOwnerState::cancelled
                                               : sadx::GameLogicOwnerState::unavailable,
                    456, {}};
            };
        }
        const auto result = sadx::run_game_reset_40bf40(runtime);
        std::cout << "reset" << case_id << ' ' << static_cast<int>(result.failure) << ' '
                  << result.source_boundary << ' ' << result.nested_source_boundary << ' '
                  << frame.value_or(123) << ' ' << state.editor.value_or(123) << ' '
                  << state.heap_at_3b290d0.value_or(123) << ' ' << state.heap_at_3b0e330.value_or(123)
                  << ' ' << random.seed_at_2bcbdf8.value_or(123) << ' ' << bool(result.pending_call);
        for (const auto &call : calls)
            std::cout << " call:" << call.source_call << ':' << call.callee << ':' << call.argument.value_or(0);
        std::cout << '\n';
    }
}

void check_native_sound_order() {
    // POD native order has no unavailable flags. Callback observes both preceding stores.
    std::array<sadx::SoundRecordInitialValues, 36> records{};
    std::array<sadx::TaskHandle, 36> tasks{};
    sadx::cleanup::AudioQueueView audio{records, tasks};
    records[0].timer = 99;
    records[0].flags = 2;
    unsigned visits = 0;
    sadx::cleanup::stop_all_sounds(audio, [&](auto) {
        if (records[0].timer || records[0].flags != 18)
            throw std::runtime_error("order");
        ++visits;
    });
    std::cout << "native-sounds " << visits << '\n';
}
} // namespace

int main() {
    check_leaf_order();
    check_reset_order();
    check_native_sound_order();
}
