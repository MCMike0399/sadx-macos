#include "engine/object_draw.hpp"
#include "game/game_mode.hpp"

namespace sadx::object_draw {
void draw_late_object(Context &, ObjectId, std::uint32_t) {
    throw flow::UnrecoveredGameFlowCall("native_late_object_renderer");
}
} // namespace sadx::object_draw
