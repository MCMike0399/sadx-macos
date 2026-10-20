#include "game/logo.hpp"
#include <cstring>

namespace sadx::logo {
namespace {
// NOTSA: INC/DEC wrap at signed limits without C++ signed overflow.
std::int32_t increment(std::int32_t value, std::uint32_t step) {
    auto bits = static_cast<std::uint32_t>(value) + step;
    std::int32_t result;
    std::memcpy(&result, &bits, sizeof(result));
    return result;
}
} // namespace
Completion advance(State &state, Calls &calls) {
    calls.run_objects();
    switch (state.mode) {
    case Mode::SegaHold: {
        const auto previous = state.frames;
        state.frames = increment(previous, 1);
        if (previous > 150) {
            state.frames = 40;
            state.mode = Mode::SegaFade;
        }
        state.logic_entry_flag = 1;
        calls.display(0);
        break;
    }
    case Mode::SegaFade: {
        const auto previous = state.frames;
        state.frames = increment(previous, static_cast<std::uint32_t>(-1));
        if (previous <= 0) {
            state.frames = 0;
            state.mode = Mode::SonicTeamHold;
        }
        calls.display(0);
        break;
    }
    case Mode::SonicTeamHold: {
        const auto previous = state.frames;
        state.frames = increment(previous, 1);
        if (previous > 150) {
            state.frames = 40;
            state.mode = Mode::SonicTeamFade;
        }
        calls.display(12);
        break;
    }
    case Mode::SonicTeamFade: {
        const auto previous = state.frames;
        state.frames = increment(previous, static_cast<std::uint32_t>(-1));
        if (previous <= 0) {
            state.frames = 0;
            state.mode = Mode::Release;
        }
        calls.display(12);
        break;
    }
    case Mode::Release:
        state.mode = Mode::Inactive;
        calls.release_textures();
        state.logic_entry_flag = 0;
        return state.demo_stage != 0 && state.demo_stage != 6 ? Completion::Title
                                                            : Completion::Movie;
    default:
        break;
    }
    return Completion::Running;
}
} // namespace sadx::logo
