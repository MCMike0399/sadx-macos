#pragma once
#include "engine/graphics_frame.hpp"
#include <map>

namespace sadx::device_lights {
// NOTSA: Direct3D keeps definitions even while a light is disabled. Draw::lights
// is its ordered enabled-light snapshot consumed by the shared Metal renderer.
struct State {
    std::map<std::uint32_t, graphics::Light> definitions;
    std::map<std::uint32_t, bool> enabled;
};
// NOTSA: native equivalents of the verified device SetLight and LightEnable writes.
void set_light(std::uint32_t, const graphics::Light &, State &, graphics::Draw &);
void enable_light(std::uint32_t, bool, State &, graphics::Draw &);
} // namespace sadx::device_lights
