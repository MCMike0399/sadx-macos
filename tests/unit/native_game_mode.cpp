#include "game/game_mode.hpp"
#include <cstdlib>
#include <iostream>
#include <string>

namespace {
void require(bool ok, const char *message) {
    if (!ok) {
        std::cerr << message << '\n';
        std::exit(1);
    }
}
std::string stopped_at(sadx::flow::GameFlowState &state) {
    try {
        sadx::flow::advance_game_mode(state);
    } catch (const sadx::flow::UnrecoveredGameFlowCall &error) {
        return error.what();
    }
    return {};
}
} // namespace

int main() {
    using namespace sadx::flow;
    GameFlowState title{};
    title.mode = GameMode::Title;
    title.pause_state = 99;
    require(stopped_at(title) == "run_title", "native title stops at its named missing callee");
    require(title.pause_state == 0, "native title publishes pause store before its callee");

    GameFlowState logo{};
    logo.mode = GameMode::LogoInit;
    require(stopped_at(logo) == "initialize_sega_logo", "native logo init is not silently skipped");
    require(logo.mode == GameMode::LogoInit, "logo mode store follows successful initialization");

    GameFlowState unused{};
    unused.mode = GameMode::Continue;
    require(stopped_at(unused) == "draw_screen_fade", "unused mode executes shared frame tail");

    GameFlowState out_of_range{};
    out_of_range.mode = static_cast<GameMode>(-1);
    require(stopped_at(out_of_range) == "draw_screen_fade", "negative mode retains default tail");
    require(static_cast<std::int32_t>(out_of_range.mode) == -1, "default leaves mode unchanged");
    std::cout
        << "Native game mode: plain state, prefix stores and explicit named STUB stops passed.\n";
}
