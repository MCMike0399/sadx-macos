#include "runtime/native_display.hpp"
#include "game/game_mode.hpp"

namespace sadx::native {
void Display::draw_selection_question(selection_question::Pose pose, const std::uint32_t &ticks,
                                      selection_question::Assets assets,
                                      const ModelDrawEnvironment &environment,
                                      const std::function<void(std::uint32_t)> &select_texture) {
    // NOTSA: the source callback borrows the display's existing matrix and device.
    struct Calls final : selection_question::Calls {
        Display &display;
        const ModelDrawEnvironment &environment;
        const std::function<void(std::uint32_t)> &select_texture;
        Calls(Display &owner, const ModelDrawEnvironment &scene,
              const std::function<void(std::uint32_t)> &select)
            : display(owner), environment(scene), select_texture(select) {}
        void push_current_matrix() override { display.coordinates_->push_current_frame(); }
        void translate_current(const NinjaVector &position) override {
            display.coordinates_->translate_matrix(display.coordinates_->current_matrix(), position,
                                                     display.arithmetic_);
        }
        void rotate_z(std::uint16_t angle) override {
            display.coordinates_->rotate_z(display.coordinates_->current_matrix(), angle,
                                             display.arithmetic_);
        }
        void rotate_x(std::uint16_t angle) override {
            display.coordinates_->rotate_x(display.coordinates_->current_matrix(), angle,
                                             display.arithmetic_);
        }
        void rotate_y(std::uint16_t angle) override {
            display.coordinates_->rotate_y(display.coordinates_->current_matrix(), angle,
                                             display.arithmetic_);
        }
        void enable_specular() override { display.device_state_.render_states[29] = 1; }
        void select_menu_texture(std::uint32_t texture) override { select_texture(texture); }
        void draw_object(std::uint32_t id) override { display.draw_object_graph(id, environment); }
        void pop_matrix(std::uint32_t count) override {
            for (std::uint32_t index = 0; index < count; ++index)
                display.coordinates_->end_frame();
        }
    } calls(*this, environment, select_texture);
    selection_question::execute(pose, ticks, assets, calls);
}
} // namespace sadx::native
