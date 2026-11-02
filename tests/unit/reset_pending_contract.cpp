#if __has_include("guest/game_reset_reference.hpp")
#include "guest/game_reset_reference.hpp"
using FrameFact = sadx::reference::FrameMessageFact;
#else
#include "game/game_reset.hpp"
using FrameFact = std::optional<std::uint32_t>;
#endif
#include <cassert>
#include <iostream>

int main() {
    using namespace sadx;
    for (unsigned outcome = 0; outcome < 4; ++outcome) {
        for (unsigned fail_at = 1; fail_at <= 28; ++fail_at) {
            FrameFact frame = 4294967295U;
            CrtRandState rng;
            GameResetState state{frame, 32768, 4096, 4096, rng};
            unsigned calls = 0;
            std::optional<GameResetCall> last;
            GameResetRuntime runtime{state, [&](const GameResetCall &call) {
                ++calls;
                last = call;
                return GameLogicOwnerResult{
                    calls == fail_at ? static_cast<GameLogicOwnerState>(outcome)
                                     : GameLogicOwnerState::completed, 12345, {}};
            }};
            const auto result = run_game_reset_40bf40(runtime);
            if (!result) {
                assert(result.pending_call && last);
                assert(result.pending_call->source_call == last->source_call);
                assert(result.pending_call->callee == last->callee);
                assert(result.pending_call->argument == last->argument);
                assert(result.nested_source_boundary == 12345);
            }
            std::cout << outcome << ' ' << fail_at << ' ' << calls << ' '
                      << static_cast<unsigned>(result.failure) << ' ' << result.source_boundary
                      << ' ' << result.nested_source_boundary << ' ' << result.completed << ' '
                      << bool(result.pending_call);
            if (result.pending_call) {
                const auto &pending = *result.pending_call;
                std::cout << ' ' << pending.source_call << ' ' << pending.callee << ' '
                          << bool(pending.argument) << ' ' << pending.argument.value_or(0);
            }
            std::cout << ' ' << bool(rng.seed_at_2bcbdf8) << ' '
                      << rng.seed_at_2bcbdf8.value_or(0) << '\n';
        }
    }
    FrameFact frame;
    CrtRandState rng;
    GameResetState state{frame, {}, 0, 0, rng};
    GameResetRuntime runtime{state, {}};
    const auto result = run_game_reset_40bf40(runtime);
    assert(result.pending_call && !result.pending_call->argument && !frame);
    std::cout << "missing " << result.pending_call->source_call << ' '
              << result.pending_call->callee << ' ' << result.source_boundary << '\n';
}
