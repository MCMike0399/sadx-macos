#include "runtime/native_display.hpp"
#include "engine/vector_normalize.hpp"

namespace sadx::lighting {
void normalize_stage_direction(std::array<float, 3> &output, const std::array<float, 3> &input, SseContext &fp) {
    normalize_vector(output, input, fp);
}
} // namespace sadx::lighting

namespace sadx::native {
lighting::DeviceServices Display::lighting_services() {
    return {
        [this](std::uint32_t index, const graphics::Light &light) {
            // [platform:graphics] Publish source light definitions to the native draw state.
            device_lights::set_light(index, light, device_lights_, device_state_);
        },
        [this](std::uint32_t index, bool enabled) {
            // [platform:graphics] Preserve disabled definitions and snapshot enabled lights.
            device_lights::enable_light(index, enabled, device_lights_, device_state_);
        },
        [this](std::uint32_t state, std::uint32_t value) {
            device_state_.render_states[state] = value;
        }};
}
void Display::set_lighting_palette(std::int32_t selection, lighting::State &scene,
                                   lighting::StageView stage, lighting::TimeServices &time) {
    lighting::PaletteView view{scene, stage, frame_mode_.pending_logic_ticks,
        queue_state_.values.lighting_palette,
        queue_state_.values.lighting_copy, lighting_device_, *coordinates_, sse_};
    lighting::set_palette(selection, view, time, lighting_services(), arithmetic_);
}
} // namespace sadx::native
