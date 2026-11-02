#if __has_include("guest/game_reset_leaves_reference.hpp")
#include "guest/game_reset_leaves_reference.hpp"
#else
#include "game/game_reset_leaves.hpp"
#endif
#include <cassert>
#include <iomanip>
#include <iostream>
#include <vector>

namespace {
enum class MediaScenario : unsigned {
    AudioNotInitialized,
    MissingReadiness,
    MissingMusicConfiguration,
    MissingVoiceConfiguration,
    InvalidateReadinessDuringCreation,
    DisableVoiceDuringMusicCreation,
    FailedCreation,
    CompletedSurveyCreation,
};

struct Fixture {
    std::optional<std::uint32_t> depth = 9, queue = 8, ready = 1;
    std::optional<std::int32_t> timing = 7;
    std::array<std::optional<std::uint32_t>, 36> records{};
    sadx::ResetLeafState state{5, 6, depth, queue, 4, 3, timing, 0, records, {}, {}, ready,
                              10, 11, 12, 13, 14, 15, 1, 1, 16, {}, 17, 18, 19, {}, {}, {}};
    Fixture() {
        state.entry_words_at_3d08428.fill(0);
    }
};

void word(const std::optional<std::uint32_t> &value) {
    std::cout << ' ' << bool(value) << ':' << value.value_or(0);
}

void check_media_initialization() {
    using namespace sadx;
    for (unsigned case_id = 0; case_id < 8; ++case_id) {
        const auto scenario = static_cast<MediaScenario>(case_id);
        Fixture fixture;
        if (scenario == MediaScenario::AudioNotInitialized)
            fixture.ready = 0;
        if (scenario == MediaScenario::MissingReadiness)
            fixture.ready.reset();
        if (scenario == MediaScenario::MissingMusicConfiguration)
            fixture.state.config_at_3a7cf3c.reset();
        if (scenario == MediaScenario::MissingVoiceConfiguration)
            fixture.state.config_at_38f6ec0.reset();
        std::vector<ResetLeafCall> calls;
        const auto result = run_reset_leaf(0x4253c0, {}, fixture.state, [&](const auto &call) {
            calls.push_back(call);
            if (scenario == MediaScenario::InvalidateReadinessDuringCreation)
                fixture.ready.reset();
            if (scenario == MediaScenario::DisableVoiceDuringMusicCreation)
                fixture.state.config_at_38f6ec0 = 0;
            return GameLogicOwnerResult{
                scenario == MediaScenario::FailedCreation ? GameLogicOwnerState::failed
                                                          : GameLogicOwnerState::completed,
                123, {}};
        });
        if (scenario == MediaScenario::CompletedSurveyCreation) {
            assert(result.outcome == ResetLeafOutcome::completed);
            assert(!fixture.state.player_at_3abdf94 && !fixture.state.player_at_3abdf9c);
            assert(fixture.state.word_at_3abdfa0 == 0 && fixture.state.word_at_3abdfa4 == 0 &&
                   fixture.state.word_at_3abdf98 == 0 && fixture.state.word_at_3abdfa8 == 0);
        }
        std::cout << "init " << case_id << ' ' << static_cast<unsigned>(result.outcome) << ' '
                  << result.source_boundary << ' ' << result.nested.source_boundary;
        for (auto *value : {&fixture.state.player_at_3abdf94, &fixture.state.player_at_3abdf9c,
                            &fixture.state.word_at_3abdfa0, &fixture.state.word_at_3abdfa4,
                            &fixture.state.word_at_3abdf98, &fixture.state.word_at_3abdfa8})
            word(*value);
        for (const auto &call : calls) {
            std::cout << " call:" << call.source_call << ':' << call.callee << ':'
                      << bool(call.argument) << ':' << call.argument.value_or(0);
        }
        std::cout << '\n';
    }
}

void check_original_vibration() {
    using namespace sadx;
    for (std::uint32_t entry : {0U, 1U, 2U, 3U, 4294967295U}) {
        Fixture fixture;
        fixture.state.entry_words_at_3d08428[0] = entry;
        const auto result = run_reset_leaf(0x785330, 4294967295U, fixture.state, {});
        assert(result.outcome == ResetLeafOutcome::completed);
        std::cout << "vibration " << entry << ' ' << result.eax.value_or(123);
        word(fixture.state.entry_words_at_3d08428[0]);
        std::cout << '\n';
    }
}

void check_debug_and_depth() {
    using namespace sadx;
    Fixture fixture;
    assert(reset_leaf_print_debug_401000("literal\n", fixture.state));
    assert(!reset_leaf_print_debug_401000("%d", fixture.state));
    std::cout << "debug " << std::quoted(fixture.state.debug_text_at_38f6d08) << '\n';
    const auto missing = run_reset_leaf(0x401410, {}, fixture.state, {});
    assert(missing.outcome == ResetLeafOutcome::unknown_state && fixture.depth == 9);
    const auto written = run_reset_leaf(0x401410, 4294967295U, fixture.state, {});
    assert(written.outcome == ResetLeafOutcome::completed && fixture.depth == 4294967295U);
}
} // namespace

int main() {
    check_media_initialization();
    check_original_vibration();
    check_debug_and_depth();
}
