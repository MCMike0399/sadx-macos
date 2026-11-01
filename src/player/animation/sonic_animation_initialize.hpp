#pragma once

#include "player/animation/sonic_animation_content.hpp"

namespace sadx::animation::detail {
// NOTSA: the producer writes these six semantic fields in several source orders.
// The sink publishes each write immediately, so retry retains every untouched old row.
template <class Sink, class Action>
void publish_sonic_row(Sink &sink, std::uint16_t index, const SonicRecipe &recipe,
                       const Action &action) {
    if (index == 0 || index == 1) {
        sink.action(index, action);
        sink.object_count(index, recipe.object_count);
        sink.transition(index, recipe.transition_step);
        sink.speed(index, recipe.frame_step);
        sink.mode(index, recipe.mode);
        sink.next(index, recipe.next);
        return;
    }
    switch (index) {
    case 84:
    case 93:
    case 110:
    case 127:
    case 132:
    case 133:
        sink.next(index, recipe.next);
        sink.action(index, action);
        sink.object_count(index, recipe.object_count);
        sink.mode(index, recipe.mode);
        break;
    case 109:
    case 128:
    case 129:
    case 142:
    case 144:
        sink.action(index, action);
        sink.next(index, recipe.next);
        sink.object_count(index, recipe.object_count);
        sink.mode(index, recipe.mode);
        break;
    case 137:
    case 138:
    case 139:
        sink.action(index, action);
        sink.mode(index, recipe.mode);
        sink.object_count(index, recipe.object_count);
        sink.next(index, recipe.next);
        break;
    case 141:
        sink.action(index, action);
        sink.mode(index, recipe.mode);
        sink.next(index, recipe.next);
        sink.object_count(index, recipe.object_count);
        break;
    case 146:
        sink.mode(index, recipe.mode);
        sink.action(index, action);
        sink.object_count(index, recipe.object_count);
        sink.next(index, recipe.next);
        break;
    default:
        sink.action(index, action);
        sink.object_count(index, recipe.object_count);
        sink.mode(index, recipe.mode);
        if (index != 94 && index != 126 && index != 130 && index != 140)
            sink.next(index, recipe.next);
        break;
    }
    sink.transition(index, recipe.transition_step);
    sink.speed(index, recipe.frame_step);
    if (index == 94 || index == 126 || index == 130 || index == 140)
        sink.next(index, recipe.next);
}

// NOTSA: one content initializer serves native import and the reference fact adapter.
template <class Sink> void initialize_sonic_rows(Sink &sink) {
    for (std::uint16_t index = 0; index < kSonicRecipes.size(); ++index) {
        const auto &recipe = kSonicRecipes[index];
        const auto action = recipe.action_export ? sink.load_action(index, recipe.action_index)
                                                 : sink.null_action();
        publish_sonic_row(sink, index, recipe, action);
        sink.complete_row();
    }
}
} // namespace sadx::animation::detail
