#pragma once
#include "player/event_actions.hpp"
#include "player/player_initialization.hpp"
#include "game/selection_preview.hpp"
#include "runtime/character_selection_assets.hpp"

namespace sadx::native {
// NOTSA: semantic field bindings borrow the shared primary owner. Root supplies
// its canonical secondary-mode store and counter reference, with adapter diagnostics.
struct EventPrimaryFields {
    event_actions::InputModeStore store_secondary_mode;
    std::function<std::int32_t &(PlayerTaskState &)> counter;
};
// NOTSA: borrowed owners for one invocation; no actor, pose, motion or list copy.
struct EventActionsContext final : event_actions::ActionSource {
    EventActionsContext(TaskWorkArena &arena, event_actions::State &state,
                        player_initialization::Registry &registry,
                        const character_pack::CharacterPack &pack, const EventPrimaryFields &fields)
        : core{arena, state, *this}, registry(registry), pack(pack), fields(fields) {}
    event_actions::Context core;
    player_initialization::Registry &registry;
    const character_pack::CharacterPack &pack;
    const EventPrimaryFields &fields;
    character_pack::ResourceId object(ActionHandle) override;
    character_pack::ResourceId motion(ActionHandle) override;
};
struct CapturedEventPrimary {
    std::shared_ptr<PlayerTaskState> state;
    TaskWorkHandle allocation;
};
// NOTSA: capture both canonical identities before a lower call can rebind the task.
CapturedEventPrimary capture_event_primary(const TaskHandle &);
PlayerEventActionNode *action_head(EventActionsContext &, const CapturedEventPrimary &);
// NOTSA: preview texture identities are supplied by the canonical character catalog.
// A new instance per update captures the primary allocation before any lower call.
class PreviewActionCalls final : public selection_preview::ActionCalls {
  public:
    PreviewActionCalls(EventActionsContext &context, const CharacterSelectionAssets &assets,
                       const std::array<regular_textures::List *, 6> &textures)
        : context_(context), assets_(assets), textures_(textures) {}
    std::shared_ptr<PlayerTaskState> primary(const TaskHandle &) override;
    void set_input(std::uint8_t, std::int8_t) override;
    std::int32_t &miles_counter() override;
    ActionHandle action(std::size_t, std::int32_t) override;
    regular_textures::List &textures(std::size_t) override;
    std::int32_t link_frames(std::size_t, std::int32_t) override;
    void set_action(const TaskHandle &, ActionHandle, regular_textures::List &,
                    float, bool, std::int32_t) override;
    PlayerEventActionNode *action_head(const std::shared_ptr<PlayerTaskState> &) override;
    PlayerEventActionNode *next_action(PlayerEventActionNode *) override;
  private:
    EventActionsContext &context_;
    const CharacterSelectionAssets &assets_;
    const std::array<regular_textures::List *, 6> &textures_;
    CapturedEventPrimary captured_;
};
} // namespace sadx::native
