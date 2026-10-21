#include "game/message_frame.hpp"
#include "cpu/fidelity_float.hpp"
#include <stdexcept>
#include <cstring>
namespace sadx::message_frame {
namespace {
// NOTSA: recovered background arithmetic, kept separate from the texture sprite
// stores.
void draw_background(const menu_caption::Context &context, StateView state, Calls &calls,
                     X87Context &fp) {
    using fidelity::ExactFloat;
    if ((context.background >> 24U) == 0)
        return;
    if (context.kind == 2) {
        calls.set_panel_color(context.background);
        const auto height = (ExactFloat::integer(fp, context.height + 4) * context.scale).rounded();
        const auto width = (ExactFloat::integer(fp, context.width + 4) * context.scale).rounded();
        calls.draw_panel(static_cast<float>(context.x), static_cast<float>(context.y), 10.0f, width,
                         height);
        return;
    }
    const auto y = ExactFloat::integer(fp, context.y);
    const auto top = y * state.vertical_scale;
    const auto height = ExactFloat::integer(fp, context.height + 4) * context.scale;
    const auto bottom =
        context.kind == 3 ? ((y - 240.0f) + ExactFloat(fp, state.vertical_scale) * 240.0f) + height
                          : height + top;
    const auto right = ((ExactFloat(fp, context.scale) * 640.0f - 320.0f) +
                        ExactFloat(fp, state.horizontal_scale) * 320.0f)
                           .rounded();
    const auto bottom_store = bottom.rounded();
    const auto top_store = (top - 4.0f).rounded();
    const auto left = (ExactFloat(fp, state.horizontal_scale) * 320.0f - 320.0f).rounded();
    calls.draw_box(left, top_store, right, bottom_store, -1.2f, context.background);
}
} // namespace
void render(StateView state, Calls &calls, X87Context &fp) {
    using fidelity::ExactFloat;
    const auto current_fov = calls.current_fov();
    // NOTSA: copy all source Angle bits into the canonical signed word.
    std::memcpy(&state.saved_fov, &current_fov, sizeof(current_fov));
    calls.set_depth_comparison(7);
    calls.set_depth_write(false);
    calls.set_fov(12743);
    auto remaining = state.queue.count;
    while (true) {
        --remaining;
        const auto index = static_cast<std::int32_t>(remaining);
        if (index < 0)
            break;
        if (static_cast<std::size_t>(index) >= state.queue.entries.size())
            throw Error(Boundary::QueueStorage,
                        "message render count exceeds its four-context queue");
        const auto *entry = state.queue.entries[static_cast<std::size_t>(index)];
        if (!entry)
            break;
        auto &context = calls.resolve(entry);
        if (context.kind == 2) {
            calls.default_blending();
        } else {
            calls.set_blend(0, 8);
            calls.set_blend(1, 6);
        }
        draw_background(context, state, calls, fp);
        if ((context.flags & 2U) != 0)
            calls.upload_texture(context);
        if (context.texture_names_attached) {
            auto &animation = calls.allocate_animation(20);
            animation = {static_cast<std::int16_t>(context.buffer_width),
                         static_cast<std::int16_t>(context.buffer_height),
                         0,
                         0,
                         0,
                         0,
                         255,
                         255,
                         0,
                         0};
            const SpriteInput sprite{static_cast<float>(context.x),
                                     static_cast<float>(context.y),
                                     context.scale,
                                     context.scale,
                                     &context.textures,
                                     &animation};
            std::array<float, 4> material{};
            for (unsigned component = 0; component < material.size(); ++component) {
                const auto channel = (context.foreground >> ((3U - component) * 8U)) & 255U;
                material[component] =
                    (ExactFloat::integer(fp, static_cast<std::int32_t>(channel)) * 0.0039215689f)
                        .rounded();
            }
            calls.set_material(material);
            calls.publish_centered_sprite(sprite);
            const auto y = context.kind == 3
                               ? (ExactFloat(fp, state.vertical_scale) * 240.0f +
                                  (ExactFloat(fp, sprite.y) - 240.0f))
                                     .rounded()
                               : (ExactFloat(fp, state.vertical_scale) * sprite.y).rounded();
            calls.set_sprite_y(y);
            calls.submit_sprite(0, 22046.900390625f, 34, 0);
            calls.set_material({0, 0, 0, 0});
        }
    }
    if (state.saved_fov != 0)
        calls.set_fov(state.saved_fov);
    calls.set_depth_comparison(1);
    calls.set_depth_write(true);
    state.queue.count = 0;
    calls.complete_caption_frame();
}
} // namespace sadx::message_frame
