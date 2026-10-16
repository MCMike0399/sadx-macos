#include "engine/ninja_queue_initial_state.hpp"
#include "engine/ninja_render.hpp"
#include <iostream>
#include <stdexcept>
int main() {
    auto values = sadx::ninja_queue_initial_state();
    sadx::NinjaRenderState render{};
    render.packed_blend_at_3ab9890 = values.packed_blend;
    const auto source = sadx::ninja_set_blend(0, 8, render);
    if (source.failure != sadx::NinjaRenderFailure::missing_tables ||
        render.packed_blend_at_3ab9890 != 8)
        throw std::runtime_error(
            "cold blend state stores the source nibble before missing renderer tables");
    const auto destination = sadx::ninja_set_blend(1, 6, render);
    if (destination.failure != sadx::NinjaRenderFailure::missing_tables ||
        render.packed_blend_at_3ab9890 != 104)
        throw std::runtime_error("destination blend preserves the source nibble");
    const auto depth = sadx::ninja_set_depth_comparison(
        static_cast<std::uint32_t>(values.depth_comparison), nullptr, render);
    if (depth.failure != sadx::NinjaRenderFailure::unknown_device || render.device_at_3d128b0)
        throw std::runtime_error("numeric defaults must not manufacture a device");
    values.reset_source[0] = -1;
    if (values.reset_capture[0] || sadx::ninja_queue_initial_state().reset_source[0] != -3000.0F)
        throw std::runtime_error("copied parameter groups and sessions are independent");
    std::cout
        << "Queue defaults: blend nibbles, device boundary and independent parameters passed.\n";
}
