#include "runtime/native_event_actions_context.hpp"
#include "runtime/native_character_resources.hpp"

namespace sadx::native {
EventPrimaryFields event_primary_fields() {
    return {
        [](PlayerTaskState &primary, std::uint8_t value) {
            auto &[orientation, index, character, event, position, path, id, scale, mode,
                   secondary_mode, timer, counter] = primary;
            secondary_mode = value;
        },
        [](PlayerTaskState &primary) -> std::int32_t & {
            auto &[orientation, index, character, event, position, path, id, scale, mode,
                   secondary_mode, timer, counter] = primary;
            if (!counter)
                throw event_actions::Error(event_actions::Boundary::RegisteredCounter);
            return *counter;
        }};
}

std::array<regular_textures::List *, 6>
preview_texture_bindings(const CharacterSelectionAssets &assets, CharacterResources &characters) {
    std::array<regular_textures::List *, 6> result{};
    for (std::size_t slot = 0; slot < result.size(); ++slot) {
        const auto character = assets.characters[slot];
        if (assets.sequences[slot].character != character)
            throw selection_preview::Error(selection_preview::Boundary::TextureCharacter, slot,
                                           "preview sequence and character declarations differ");
        const auto kind = static_cast<selection_preview::ActorKind>(slot);
        const std::size_t binding = kind == selection_preview::ActorKind::Knuckles ||
                                           kind == selection_preview::ActorKind::Big
                                       ? 1 : 0;
        if (character >= characters.lists().size() ||
            binding >= characters.lists()[character].size())
            throw selection_preview::Error(selection_preview::Boundary::TextureBinding, slot,
                                           "source preview texture declaration is unavailable");
        result[slot] = &characters.lists()[character][binding];
    }
    return result;
}

character_pack::ResourceId EventActionsContext::object(ActionHandle handle) {
    if (!handle.identity)
        throw event_actions::Error(event_actions::Boundary::ActionObject);
    return pack.resolve(handle).object;
}
character_pack::ResourceId EventActionsContext::motion(ActionHandle handle) {
    if (!handle.identity)
        throw event_actions::Error(event_actions::Boundary::ActionMotion);
    return pack.resolve(handle).motion;
}
CapturedEventPrimary capture_event_primary(const TaskHandle &task) {
    if (!task)
        return {};
    return {task->primary_state, task->primary_work};
}
PlayerEventActionNode *action_head(EventActionsContext &context, const CapturedEventPrimary &primary) {
    if (!primary.state)
        throw event_actions::Error(event_actions::Boundary::Primary);
    return event_actions::action_head(context.core.arena, primary.allocation);
}
std::shared_ptr<PlayerTaskState> PreviewActionCalls::primary(const TaskHandle &task) {
    captured_ = capture_event_primary(task);
    return captured_.state;
}
void PreviewActionCalls::set_input(std::uint8_t player, std::int8_t mode) {
    event_actions::set_input(context_.registry.primary, player, mode,
                             context_.fields.store_secondary_mode);
}
std::int32_t &PreviewActionCalls::miles_counter() {
    const auto primary = context_.registry.primary[1];
    if (!primary || !context_.fields.counter)
        throw event_actions::Error(event_actions::Boundary::RegisteredCounter);
    return context_.fields.counter(*primary);
}
ActionHandle PreviewActionCalls::action(std::size_t actor, std::int32_t index) {
    const auto &sequence = assets_.sequences.at(actor).steps;
    if (index == static_cast<std::int32_t>(sequence.size()))
        return {};
    if (index < 0)
        throw std::out_of_range("negative preview action index");
    return sequence.at(static_cast<std::size_t>(index)).action;
}
regular_textures::List &PreviewActionCalls::textures(std::size_t actor) {
    auto *list = textures_.at(actor);
    if (!list)
        throw selection_preview::Error(selection_preview::Boundary::TextureBinding, actor,
                                       "preview texture list is not bound");
    return *list;
}
std::int32_t PreviewActionCalls::link_frames(std::size_t actor, std::int32_t index) {
    if (index < 0)
        throw std::out_of_range("negative preview link index");
    return assets_.sequences.at(actor).steps.at(static_cast<std::size_t>(index)).link_frames;
}
void PreviewActionCalls::set_action(const TaskHandle &task, ActionHandle action,
                                    regular_textures::List &textures, float speed,
                                    bool loop, std::int32_t link) {
    event_actions::set_action(context_.core, task, action, &textures, speed, loop ? 1 : 0, link);
}
PlayerEventActionNode *PreviewActionCalls::action_head(const std::shared_ptr<PlayerTaskState> &primary) {
    if (primary != captured_.state)
        throw event_actions::Error(event_actions::Boundary::Primary);
    return native::action_head(context_, captured_);
}
PlayerEventActionNode *PreviewActionCalls::next_action(PlayerEventActionNode *node) {
    return event_actions::next_action(context_.core.arena, node);
}
} // namespace sadx::native
