#include "engine/render_state_snapshot.hpp"

namespace sadx::render_snapshot {
void save_control(View view) { view.saved.control = view.control; }
void load_control(View view) { view.control = view.saved.control; }
void save_attributes(View view) {
    const auto mask = view.mask;
    const auto attributes = view.attributes;
    view.saved.mask = mask;
    view.saved.attributes = attributes;
}
void load_attributes(View view) {
    constexpr std::uint32_t kConstantAttributeEnabled = 0x00800000;
    const auto attributes = view.saved.attributes;
    const auto mask = view.saved.mask;
    view.mask = mask;
    view.attributes = attributes | kConstantAttributeEnabled;
}
void clear_attributes(View view, std::uint32_t mask, std::uint32_t attributes) {
    const auto next_mask = view.mask & ~mask;
    const auto current_attributes = view.attributes;
    view.mask = next_mask;
    view.attributes = current_attributes & ~attributes;
}
void set_control(View view, std::uint32_t control) { view.control = control; }
void set_material(State &state, const std::array<float, 4> &material, queue::State &queue,
                  const queue::Services &services, X87Context &arithmetic) {
    // The source publishes the scratch color before delegating its clamp/device work.
    state.material[0] = material[0];
    state.material[1] = material[1];
    state.material[2] = material[2];
    state.material[3] = material[3];
    queue::set_material(state.material, queue, services, arithmetic);
}
void set_object_callback(State &state, ObjectCallback callback) { state.object_callback = callback; }
} // namespace sadx::render_snapshot
