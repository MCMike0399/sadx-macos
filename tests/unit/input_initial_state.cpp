#include "input/input_initial_state.hpp"
#include <iostream>
#include <stdexcept>

int main() {
    auto first = sadx::input::input_initial_state();
    const auto second = sadx::input::input_initial_state();
    // The load-time data predates polling, which later writes the held complement.
    if (first.controller_template.not_held != 0 || first.dream_pads[0].not_held != 0)
        throw std::runtime_error("load-time values must not apply the post-poll complement");
    first.mod_raw[0].held = 7;
    first.dream_pads[0].previous_held = 9;
    first.edges.raw[59][0] = 1;
    if (first.engine_raw[0].held || first.mod_raw[1].held || second.mod_raw[0].held ||
        second.dream_pads[0].previous_held || second.edges.raw[59][0])
        throw std::runtime_error(
            "native input collections and sessions must own independent values");
    std::cout << "Input initial state: pre-poll complement and independent records passed.\n";
}
