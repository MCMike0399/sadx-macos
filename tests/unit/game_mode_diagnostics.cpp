#include "guest/game_mode_reference.hpp"
#include <array>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace {
using namespace sadx;
void require(bool value, const char *message) {
    if (!value) { std::cerr << message << '\n'; std::exit(1); }
}
bool same(const GameModeCall &a, const GameModeCall &b) {
    return a.source_call == b.source_call && a.callee == b.callee && a.arguments == b.arguments;
}
struct Fixture {
    GameLoopState loop{};
    std::optional<std::int32_t> mode = 8;
    GameModeState state{};
    GameModeRuntime runtime{loop, mode, state, {}};
    std::vector<GameModeCall> calls;
    Fixture() {
        state.word_at_3b2c45c = 4294967294U;
        state.word_at_3b290e4 = 0;
        runtime.invoke = [&](const GameModeCall &call) {
            calls.push_back(call);
            return GameModeOwnerResult{{GameLogicOwnerState::completed, 77},
                call.callee == 0x42f020 ? std::optional<std::uint32_t>(1) : std::nullopt};
        };
    }
};
void emit(const char *name, const GameModeResult &result) {
    std::cout << name << ' ' << static_cast<unsigned>(result.failure) << ' '
        << result.source_boundary << ' ' << result.nested_source_boundary << ' '
        << result.completed << ' ' << result.pending_call.has_value();
    if (result.pending_call) {
        const auto &call = *result.pending_call;
        std::cout << ' ' << call.source_call << ' ' << call.callee;
        for (const auto &arg : call.arguments)
            std::cout << ' ' << arg.has_value() << ' ' << arg.value_or(0);
    }
    std::cout << '\n';
}
void boundaries() {
    Fixture full;
    const auto completed = run_game_mode_40c090(full.runtime);
    require(completed.completed && completed.failure == GameModeFailure::none &&
            completed.source_boundary == 0 && completed.nested_source_boundary == 0 &&
            !completed.pending_call, "successful calls must clear pending and nested diagnostics");
    require(full.calls.size() == 6 && full.mode == 11 && full.state.byte_at_3abdcfc == 0,
            "complete movie cleanup prefix");
    emit("complete", completed);
    for (std::size_t n = 1; n <= full.calls.size(); ++n) {
        for (const auto status : {GameLogicOwnerState::failed, GameLogicOwnerState::cancelled,
                                  GameLogicOwnerState::unavailable}) {
            Fixture fixture;
            fixture.runtime.invoke = [&](const GameModeCall &call) {
                fixture.calls.push_back(call);
                if (fixture.calls.size() == n)
                    return GameModeOwnerResult{{status, 49387U + static_cast<std::uint32_t>(n)}, {}};
                return GameModeOwnerResult{{GameLogicOwnerState::completed, 77},
                    call.callee == 0x42f020 ? std::optional<std::uint32_t>(1) : std::nullopt};
            };
            const auto result = run_game_mode_40c090(fixture.runtime);
            const auto expected = status == GameLogicOwnerState::failed ? GameModeFailure::owner_failed :
                status == GameLogicOwnerState::cancelled ? GameModeFailure::cancelled : GameModeFailure::missing_owner;
            require(!result.completed && result.failure == expected && result.pending_call &&
                result.source_boundary == full.calls[n - 1].source_call &&
                result.nested_source_boundary == 49387U + n &&
                same(*result.pending_call, full.calls[n - 1]), "complete pending diagnostic tuple changed");
            require(fixture.calls.size() == n, "reference owner failure must retain its exact call prefix");
            for (std::size_t i = 0; i < n; ++i)
                require(same(fixture.calls[i], full.calls[i]), "reference argument prefix changed");
            emit("owner-prefix", result);
        }
    }
    Fixture missing;
    missing.runtime.invoke = {};
    auto result = run_game_mode_40c090(missing.runtime);
    require(result.failure == GameModeFailure::missing_owner && result.source_boundary == 0x40c2ef &&
            result.nested_source_boundary == 0 && result.pending_call &&
            result.pending_call->arguments[0] == 4294967294U &&
            !result.pending_call->arguments[1] && !result.pending_call->arguments[2],
            "missing owner must retain argument presence and high word bits");
    emit("missing-owner", result);
    Fixture no_return;
    no_return.runtime.invoke = [](const GameModeCall &) {
        return GameModeOwnerResult{{GameLogicOwnerState::completed, 291}, {}};
    };
    result = run_game_mode_40c090(no_return.runtime);
    require(result.failure == GameModeFailure::missing_return && !result.completed &&
            result.source_boundary == 0x40c2ef && result.nested_source_boundary == 291 &&
            result.pending_call && same(*result.pending_call, full.calls[0]),
            "missing required return retains its nested boundary and exact pending payload");
    emit("missing-return", result);
    Fixture zero;
    zero.state.word_at_3b2c45c = 0; zero.runtime.invoke = {};
    result = run_game_mode_40c090(zero.runtime);
    require(result.pending_call && result.pending_call->arguments[0] == 0 &&
            !result.pending_call->arguments[1], "known zero argument must stay distinct from missing argument");
    emit("known-zero", result);
}
void missingness_and_binding() {
    Fixture unknown;
    unknown.state.word_at_3b2c45c.reset();
    auto result = run_game_mode_40c090(unknown.runtime);
    require(result.failure == GameModeFailure::unknown_state && result.source_boundary == 0x40c2e9 &&
            result.nested_source_boundary == 0 && !result.pending_call && unknown.calls.empty(),
            "missing movie fact must stop before owner dispatch");
    emit("unknown-movie", result);
    unknown.mode.reset();
    result = run_game_mode_40c090(unknown.runtime);
    require(result.failure == GameModeFailure::unknown_state && result.source_boundary == 0x40c091 &&
            !result.pending_call && result.nested_source_boundary == 0,
            "fresh invocation must not retain prior pending state");
    emit("unknown-mode", result);
    Fixture later;
    later.runtime.invoke = [&](const GameModeCall &call) {
        later.calls.push_back(call);
        later.state.word_at_3b290e4.reset();
        return GameModeOwnerResult{{GameLogicOwnerState::completed, 49387},
            call.callee == 0x42f020 ? std::optional<std::uint32_t>(1) : std::nullopt};
    };
    result = run_game_mode_40c090(later.runtime);
    require(result.failure == GameModeFailure::unknown_state && result.source_boundary == 0x40c95f &&
            !result.pending_call && result.nested_source_boundary == 0 && later.calls.size() == 4 &&
            later.mode == 11 && later.state.byte_at_3abdcfc == 0,
            "callback-cleared fact must stop lazily after reached stores, with no stale pending call");
    emit("callback-cleared-fact", result);
    Fixture outer;
    bool reentered = false;
    outer.runtime.invoke = [&](const GameModeCall &call) {
        outer.calls.push_back(call);
        if (!reentered) {
            reentered = true;
            Fixture nested;
            nested.runtime.invoke = [](const GameModeCall &) -> GameModeOwnerResult {
                throw std::runtime_error("inner-host-error");
            };
            bool escaped = false;
            try { run_game_mode_40c090(nested.runtime); }
            catch (const std::runtime_error &) { escaped = true; }
            require(escaped, "host exception must propagate through the inner binding");
        }
        return GameModeOwnerResult{{GameLogicOwnerState::completed, 49387},
            call.callee == 0x42f020 ? std::optional<std::uint32_t>(1) : std::nullopt};
    };
    result = run_game_mode_40c090(outer.runtime);
    require(result.completed && result.failure == GameModeFailure::none &&
            !result.pending_call && result.nested_source_boundary == 0 && outer.calls.size() == 6,
            "inner exception must restore outer reference provenance and pending state");
    emit("reentrant-unwind", result);
}
} // namespace
int main() { boundaries(); missingness_and_binding(); }
