#include "game/character_select_labels.hpp"
#include "cpu/fidelity_float.hpp"

namespace sadx::character_select_labels {
using fidelity::ExactFloat;
namespace {
// NOTSA: share the identical source centering arithmetic, retaining binary32 stores.
float centered(float position, float screen_scale, float half_extent, X87Context &fp) {
    const auto offset = ExactFloat(fp, position) - half_extent;
    const auto screen = ExactFloat(fp, screen_scale) * half_extent;
    return (offset + screen).rounded();
}
// NOTSA: explicit imported-table boundary instead of an out-of-bounds source read.
std::uint32_t texture_index(std::int32_t character, const std::array<std::uint8_t, 7> &indices,
                            Boundary boundary) {
    if (character < 0 || static_cast<std::size_t>(character) >= indices.size())
        throw Error(boundary, character);
    return indices[static_cast<std::size_t>(character)];
}
}
void draw_label(std::int32_t character, std::uint32_t color, float x, float y, float depth,
                State state, Calls &calls, X87Context &fp) {
    calls.select_texture(Texture::Labels);
    calls.material(color);
    if (character == 6 && state.variant == 1) {
        calls.select_texture(Texture::MetalName);
        const float target_y = centered(y, state.vertical_scale, 240.0f, fp);
        if (state.text_language == 0) {
            const float target_x = centered(x, state.horizontal_scale, 320.0f, fp);
            calls.draw_scaled(0, target_x, target_y, depth, 0.81f, 0.8f);
        } else {
            const auto offset_x = ExactFloat(fp, x) + 5.0f - 320.0f;
            const auto screen_x = ExactFloat(fp, state.horizontal_scale) * 320.0f;
            const float target_x = (offset_x + screen_x).rounded();
            calls.draw_scaled(1, target_x, target_y, depth, 0.78f, 0.78f);
        }
    } else {
        const auto offset_y = ExactFloat(fp, y) - 240.0f;
        const auto texture = texture_index(character, state.label_textures, Boundary::LabelIndex);
        const auto screen_y = ExactFloat(fp, state.vertical_scale) * 240.0f;
        const float target_y = (offset_y + screen_y).rounded();
        if (state.text_language == 0) {
            const float target_x = centered(x, state.horizontal_scale, 320.0f, fp);
            calls.draw_texture(texture, target_x, target_y, depth);
        } else {
            const auto offset_x = ExactFloat(fp, x) + 5.0f - 320.0f;
            const auto screen_x = ExactFloat(fp, state.horizontal_scale) * 320.0f;
            const float target_x = (offset_x + screen_x).rounded();
            calls.draw_scaled(texture, target_x, target_y, depth, 0.78f, 0.78f);
        }
    }
}
void draw_name(std::int32_t character, std::uint32_t color, float x, float y, float depth,
               float scale_x, float scale_y, State state, Calls &calls, X87Context &fp) {
    const bool metal = character == 6 && state.variant == 1;
    calls.select_texture(metal ? Texture::MetalName : Texture::Names);
    calls.material(color);
    const auto offset_y = ExactFloat(fp, y) - 240.0f;
    const auto screen_y = ExactFloat(fp, state.vertical_scale) * 240.0f;
    const auto texture = metal ? 2U : texture_index(character, state.name_textures, Boundary::NameIndex);
    const float target_y = (offset_y + screen_y).rounded();
    const float target_x = centered(x, state.horizontal_scale, 320.0f, fp);
    calls.draw_scaled(texture | 67108864U, target_x, target_y, depth, scale_x, scale_y);
}
} // namespace sadx::character_select_labels
