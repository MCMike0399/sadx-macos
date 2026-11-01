#include "runtime/native_display.hpp"
#include <array>
#include <cassert>
#include <iostream>

int main() {
    sadx::frame::ModeState timing{};
    sadx::native::Display display({}, timing);
    const std::array<sadx::pvr::Point, 4> quad{{{0, 0}, {8, 0}, {0, 8}, {8, 8}}};
    // Source CompareModes words, independently established by the original depth judge.
    constexpr std::array<std::uint32_t, 8> comparisons{1, 2, 3, 4, 5, 6, 7, 8};
    constexpr std::uint32_t kOpaqueWhite = 0xFFFFFFFF;
    auto &output = display.output();
    output.begin_frame();
    for (const auto mode : {0U, 1U, 2U, 3U, 4U, 5U, 6U, 7U, 8U, 42U, 0xFFFFFFFFU}) {
        display.set_depth_comparison(mode);
        display.set_depth_write(false);
        output.draw_polygon(quad, kOpaqueWhite, 1.0f, 0);
        const auto first_index = output.frame().commands.size() - 1;
        const auto &first = std::get<sadx::graphics::Draw>(output.frame().commands[first_index]);
        assert(first.render_states.at(23) == comparisons[mode & 7]);
        assert(first.render_states.at(14) == 0);

        display.set_depth_write(true);
        output.draw_polygon(quad, kOpaqueWhite, 1.0f, 0);
        const auto &next = std::get<sadx::graphics::Draw>(output.frame().commands.back());
        assert(next.render_states.at(23) == comparisons[mode & 7]);
        assert(next.render_states.at(14) == 1);
        // Observe snapshots through the same public draw interface as real callers.
        const auto &retained = std::get<sadx::graphics::Draw>(output.frame().commands[first_index]);
        assert(retained.render_states.at(14) == 0);
    }
    std::cout << "Native depth state: masked source modes and both write states reach actual "
                 "draw snapshots; earlier draws retain their state\n";
}
