#include "engine/material_correction.hpp"

namespace sadx::material_correction {
namespace {
constexpr float kColorChannelScale = 0.003921569f;

// NOTSA: preserve the source's load, multiply, then binary32 store for each channel.
void normalize_rgb(Context &context, std::array<float, 4> &color) {
    for (std::size_t channel = 0; channel < 3; ++channel)
        color[channel] = (exact(context, color[channel]) * kColorChannelScale).rounded();
}
} // namespace

void correct_material(Context &context) {
    auto material = get_material(context);
    // The original publishes Power after the first multiply and before its store.
    const auto ambient_red = exact(context, material.ambient[0]) * kColorChannelScale;
    material.power = specular_power(context);
    material.ambient[0] = ambient_red.rounded();
    for (std::size_t channel = 1; channel < 3; ++channel)
        material.ambient[channel] =
            (exact(context, material.ambient[channel]) * kColorChannelScale).rounded();
    normalize_rgb(context, material.diffuse);
    normalize_rgb(context, material.specular);
    set_material(context, material);
}

} // namespace sadx::material_correction
