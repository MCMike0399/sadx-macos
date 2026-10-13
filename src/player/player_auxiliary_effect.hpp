#pragma once

#include "player/player_startup.hpp"
#include <functional>

namespace sadx {

// Actual immutable9275a0: five216-byte groups, each containing three72-byte
// variants. Unlifted fields retain their source bytes; no guest pointers cast.
struct PlayerAuxiliaryEffectAssetTable {
    std::array<std::uint8_t,0x438> bytes;
    std::uint32_t character(std::size_t group) const;
};
std::shared_ptr<const PlayerAuxiliaryEffectAssetTable> decode_player_auxiliary_effect_table(const std::uint8_t*,std::size_t);
struct PlayerAuxiliaryEffectSelection {std::shared_ptr<const PlayerAuxiliaryEffectAssetTable> table;std::size_t group;};
struct PlayerAuxiliaryEffectTaskWork {
    TaskWorkHandle allocation;
    // Outer omission unknown; present-null is source counter.ptr+8 cleared.
    std::optional<std::shared_ptr<const PlayerAuxiliaryEffectSelection>> selection_at_8;
};

enum class PlayerAuxiliaryEffectFailure {
    none,missing_task,missing_primary,conflicting_view,missing_arena,work_failed,
    missing_context,unknown_registration,unknown_current_character,missing_table,
    unknown_display_switch,missing_render_state,render_state_failed,
    missing_sprite_owner,sprite_failed,missing_fp,unknown_selection,
    unsupported_variant_pointer,missing_update_owner,
    missing_secondary_mode,
    fp_failed,
};
struct PlayerAuxiliaryEffectSprite {
    NinjaVector position;
    float scale_x,scale_y;
    std::int32_t angle;
    // Source asset identity, never a native address or guessed GPU texture.
    static constexpr std::uint32_t texture_list_source=0x94a2a8;
    static constexpr std::uint32_t texture_animation_source=0x9279d8;
};
struct PlayerAuxiliaryEffectResult {
    PlayerAuxiliaryEffectFailure failure=PlayerAuxiliaryEffectFailure::none;
    std::uint32_t source_boundary=0,nested_source_boundary=0;
    TaskWorkFailure work_failure=TaskWorkFailure::none;
    std::optional<std::uint32_t> character;
    std::optional<std::size_t> selected_group;
    std::optional<PlayerAuxiliaryEffectSprite> pending_sprite;
    TaskWorkHandle pending_node;
    std::optional<X87Failure> fp_failure;
    std::uint16_t fp_exceptions=0;
    explicit operator bool()const noexcept{return failure==PlayerAuxiliaryEffectFailure::none;}
};
struct PlayerAuxiliaryEffectDiagnostic final:TaskCallbackDiagnostic {PlayerAuxiliaryEffectResult result;explicit PlayerAuxiliaryEffectDiagnostic(PlayerAuxiliaryEffectResult r):result(std::move(r)){};};

struct PlayerAuxiliaryEffectExternalResult {
    bool available;
    std::uint32_t source_boundary;
};
// Scheduler owns this context. Live registry/facts and FP state are explicit
// borrowed dependencies; mutation of this same context is observed per call.
struct PlayerAuxiliaryEffectRuntime final:TaskCallbackContext {
    const PlayerStartupRegistry* players=nullptr;
    const std::optional<std::uint16_t>* current_character_at_3b22dc0=nullptr; // borrowed; null or unset is unknown
    std::shared_ptr<const PlayerAuxiliaryEffectAssetTable> table;
    const std::optional<std::int32_t>* display_switch_at_3b1117c=nullptr; // borrowed timing owner; null is unknown
    X87Context* fp=nullptr;
    std::function<PlayerAuxiliaryEffectExternalResult(unsigned,std::uint32_t)> render_state_4030d0;
    std::function<PlayerAuxiliaryEffectExternalResult(const PlayerAuxiliaryEffectSprite&,unsigned,std::uint32_t)> sprite_4072f0;
};

TaskCallbackContextResult install_player_auxiliary_effect_context(TaskScheduler&,
    const std::shared_ptr<PlayerAuxiliaryEffectRuntime>&);
PlayerAuxiliaryEffectResult initialize_player_auxiliary_effect(TaskScheduler&,const TaskHandle&,PlayerAuxiliaryEffectRuntime&);
PlayerAuxiliaryEffectResult destroy_player_auxiliary_effect(TaskScheduler&,const TaskHandle&);
PlayerAuxiliaryEffectResult display_player_auxiliary_effect(TaskScheduler&,const TaskHandle&,PlayerAuxiliaryEffectRuntime&);
PlayerAuxiliaryEffectResult update_player_auxiliary_effect(TaskScheduler&,const TaskHandle&,PlayerAuxiliaryEffectRuntime&);
void player_auxiliary_effect_initialize_callback(TaskScheduler&,const TaskHandle&); //4a2b60→4a2ae0
void player_auxiliary_effect_update_callback(TaskScheduler&,const TaskHandle&);     //4a2570
void player_auxiliary_effect_display_callback(TaskScheduler&,const TaskHandle&);    //4a0760
void player_auxiliary_effect_destroy_callback(TaskScheduler&,const TaskHandle&);    //4a0720

} // namespace sadx
