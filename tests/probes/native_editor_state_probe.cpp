#include "game/editor_state.hpp"
extern "C" std::int32_t sadx_native_editor_state(std::uint16_t editor, std::uint16_t mission) {
    return sadx::editor::check_mode({editor, mission});
}
