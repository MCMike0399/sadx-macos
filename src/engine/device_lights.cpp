#include "engine/device_lights.hpp"

namespace sadx::device_lights {
namespace {
// NOTSA: command snapshots preserve source light-slot order and copy each value.
void publish(const State &state, graphics::Draw &draw) {
    draw.lights.clear();
    for (const auto &[index, enabled] : state.enabled) {
        if (enabled)
            draw.lights.push_back(state.definitions.at(index));
    }
}
// NOTSA: matches the reference device's Direct3D implicit-light contract.
graphics::Light implicit_light() {
    graphics::Light light;
    light.diffuse = {1, 1, 1, 0};
    light.direction = {0, 0, 1};
    return light;
}
} // namespace
void set_light(std::uint32_t index, const graphics::Light &light, State &state,
               graphics::Draw &draw) {
    state.definitions[index] = light;
    publish(state, draw);
}
void enable_light(std::uint32_t index, bool enabled, State &state, graphics::Draw &draw) {
    state.enabled[index] = enabled;
    // The reference backend creates the API default definition on either
    // enable value; disabled defaults contribute no illumination.
    if (state.definitions.count(index) == 0)
        state.definitions[index] = implicit_light();
    publish(state, draw);
}
} // namespace sadx::device_lights
