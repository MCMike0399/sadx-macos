#include "game/game_mode.hpp"
#include "runtime/model_draw_environment.hpp"

namespace sadx::native {
polybuff::BufferState &normal_uv_buffer(Display &) {
    // NOTSA: source buffer storage cannot be duplicated beside the unmigrated owner.
    throw flow::UnrecoveredGameFlowCall("native_polybuff_owner_binding");
}
} // namespace sadx::native
