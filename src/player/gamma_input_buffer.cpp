#include "player/gamma_input_buffer.hpp"

namespace sadx::gamma_input_buffer {
void restore(Access &access, std::int32_t player) {
    const auto angle = access.buffered_angle();
    access.store_angle(player, angle);
    const auto stroke = access.buffered_stroke_bits();
    access.store_stroke_bits(player, stroke);
}
} // namespace sadx::gamma_input_buffer
