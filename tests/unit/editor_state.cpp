#include "game/editor_state.hpp"
#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>

int main() {
    struct Words {
        std::uint16_t editor, upper_editor, mission, upper_mission;
    } words{0, 0xa59b, 0, 0x7183};
    const sadx::editor::View state{words.editor, words.mission};
    constexpr std::array<std::uint16_t, 9> values{0, 1, 2, 0x100, 0x7fff, 0x8000,
                                                0x8001, 0xfffe, 0xffff};
    unsigned count = 0;
    for (auto editor : values) {
        for (auto mission : values) {
            words.editor = editor;
            words.mission = mission;
            const auto sign_extend = [](std::uint16_t value) -> std::int32_t {
                return value < 32768 ? value : static_cast<std::int32_t>(value) - 65536;
            };
            assert(sadx::editor::check_mode(state) == (sign_extend(mission) | sign_extend(editor)));
            assert(words.editor == editor && words.mission == mission);
            assert(words.upper_editor == 0xa59b && words.upper_mission == 0x7183);
            ++count;
        }
    }
    std::cout << count << " signed non-normalized editor combinations; live distinct words and canaries\n";
}
