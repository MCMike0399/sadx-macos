#include "game/lighting_palette.hpp"
#include "cpu/fidelity_float.hpp"

namespace sadx::lighting {
void copy_palette_type(std::int32_t type, std::uint8_t &copy) {
    copy = static_cast<std::uint8_t>(type);
}
namespace {
// NOTSA: both source entries clear all104 scratch bytes before assigning its type.
void clear_light(graphics::Light &light) {
    light = {static_cast<graphics::LightType>(0), {}, {}, {}, {}, {}, 0, 0, {}, 0, 0};
}
// NOTSA: source FLD/FCHS/FSTP preserves its explicit x87 behavior for every component.
void reverse_direction(std::array<float, 3> &direction, X87Context &fp) {
    for (auto &component : direction)
        component = (-fidelity::ExactFloat(fp, component)).rounded();
}
// NOTSA: names the repeated device-state suffix of the two source entries.
void enable_rendering(const DeviceServices &device, bool specular) {
    device.render_state(137, 1);
    device.render_state(29, specular);
    device.render_state(27, 1);
    device.render_state(9, 2);
}
// NOTSA: source stage9/act0 rotates and publishes the shared direction.
void rotate_stage_direction(const std::array<float, 3> &direction, PaletteView view,
                            X87Context &fp) {
    auto &matrices = view.matrices;
    matrices.begin_unit_frame();
    if (view.scene.rotation_y != 0)
        matrices.rotate_y(matrices.current_matrix(),
                          static_cast<std::uint16_t>(view.scene.rotation_y), fp);
    if (view.scene.rotation_z != 0)
        matrices.rotate_z(matrices.current_matrix(),
                          static_cast<std::uint16_t>(view.scene.rotation_z), fp);
    NinjaVector transformed{};
    const auto result = matrices.calculate_vector(matrices.current_matrix(),
        {direction[0], direction[1], direction[2]}, transformed, fp);
    if (!result)
        throw CoordinateError(result.failure);
    matrices.end_ex_frame();
    view.device.stage_direction = {transformed.x, transformed.y, transformed.z};
    view.device.light.direction = view.device.stage_direction;
}
// NOTSA: names mode0's loop over the four already selected stage lights.
void apply_stage(PaletteView view, const DeviceServices &device, X87Context &fp,
                 std::int32_t selected_stage) {
    auto &light = view.device.light;
    for (std::uint32_t index = 0; index < view.scene.active_stage.size(); ++index) {
        const auto &environment = view.scene.active_stage[index];
        if ((environment.flags & 1U) == 0) {
            device.enable_light(index, false);
            continue;
        }
        if (index == 0) {
            device.render_state(139, 0);
            for (std::size_t channel = 0; channel < 3; ++channel) {
                light.ambient[channel] = environment.ambient[channel];
                light.specular[channel] = environment.specular;
            }
        }
        light.diffuse[3] = 1;
        for (std::size_t channel = 0; channel < 3; ++channel)
            light.diffuse[channel] =
                (fidelity::ExactFloat(fp, environment.color[channel]) * environment.diffuse).rounded();
        if (view.stage.stage == 9 && view.stage.act == 0)
            rotate_stage_direction(environment.direction, view, fp);
        else
            light.direction = normalize_stage_direction(environment.direction);
        device.set_light(index, light);
        device.enable_light(index, true);
        enable_rendering(device, selected_stage == 19);
        device.render_state(141, 1);
    }
}
} // namespace
void apply_palette(PaletteView view, const DeviceServices &device, X87Context &fp) {
    auto &light = view.device.light;
    const auto &environment = view.scene.active_palette;
    clear_light(light);
    for (std::size_t channel = 0; channel < 3; ++channel) {
        light.ambient[channel] = environment.ambient[channel];
        light.diffuse[channel] = environment.diffuse;
        light.specular[channel] = environment.upper_specular[channel];
    }
    light.type = graphics::LightType::Directional;
    light.diffuse[3] = 1;
    const auto direction = (environment.flags & 2U) != 0 ? environment.direction
                                                      : view.scene.active_stage[0].direction;
    if (view.stage.stage == 9 && view.stage.act == 0)
        light.direction = view.device.stage_direction;
    else {
        light.direction = direction;
        if ((environment.flags & 4U) != 0)
            reverse_direction(light.direction, fp);
    }
    device.set_light(0, light);
    device.enable_light(0, true);
    enable_rendering(device, true);
    for (std::uint32_t index = 1; index < 4; ++index) {
        if ((view.stage.stage == 8 || (environment.flags & 8U) != 0) && index == 3) {
            if (view.stage.stage == 8)
                light.specular[0] = light.specular[1] = light.specular[2] = 0.2f;
            reverse_direction(light.direction, fp);
            device.set_light(3, light);
            device.enable_light(3, true);
        } else {
            device.enable_light(index, false);
        }
    }
}
void set_palette(std::int32_t selection, PaletteView view, TimeServices &time,
                 const DeviceServices &device, X87Context &fp) {
    const auto palette_number = selection / 2;
    if (palette_number != view.palette_number) {
        view.palette_number = palette_number;
        copy_palette_type(palette_number, view.palette_copy);
    }
    clear_light(view.device.light);
    const auto selected = time_of_day_stage(view.stage, time);
    view.device.light.type = graphics::LightType::Directional;
    select_stage(static_cast<std::uint32_t>(selected.first), selected.second, view.scene);
    if (view.loop_count != 0)
        return;
    std::int32_t character;
    switch (selection) {
    case 0:
        apply_stage(view, device, fp, selected.first);
        return;
    case 2:
        character = 0;
        break;
    case 4:
        character = 6;
        break;
    case 6:
        if (selected.first == 19)
            return;
        character = 8;
        break;
    default: return;
    }
    select_palette(static_cast<std::uint32_t>(selected.first), selected.second, character, view.scene);
    apply_palette(view, device, fp);
}
} // namespace sadx::lighting
