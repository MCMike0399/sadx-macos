#pragma once
#include "runtime/native_player_system.hpp"
#include "runtime/native_display.hpp"
#include "player/gamma_display.hpp"
#include "engine/render_state_snapshot.hpp"

namespace sadx::gamma_display {
// NOTSA: one invocation borrows the captured task identities and canonical owners.
// Later publications never refresh these source locals or the current-player slots.
struct Context {
    gamma_selection::Context &owner;
    TaskHandle task;
    TaskWorkHandle primary, extra;
    std::shared_ptr<PlayerTaskState> primary_state;
    player_initialization::PlayerHandle player;
    std::shared_ptr<collision_component::Work> collision;
    explicit Context(gamma_selection::Context &owner) : owner(owner) {}
    render_snapshot::View render_state() {
        auto &display = owner.rendering;
        return {display.render_snapshot_, display.control_3d_, display.constant_attribute_mask_,
                display.constant_attributes_};
    }
    auto &matrices() { return *owner.rendering.coordinates_; }
    auto &arithmetic() { return owner.rendering.arithmetic_; }
    void material() {
        auto &display = owner.rendering;
        render_snapshot::set_material(display.render_snapshot_, {1, 1, 1, 1},
            display.queue_state_, display.queue_services(), display.arithmetic_);
    }
    void callback() {
        render_snapshot::set_object_callback(owner.rendering.render_snapshot_,
                                            render_snapshot::ObjectCallback::Gamma);
    }
    void textures();

};
} // namespace sadx::gamma_display
