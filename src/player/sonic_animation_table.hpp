#pragma once
#include "player/player_animation.hpp"
namespace sadx {
using SonicAnimationExport = std::vector<std::optional<AnimationResourceReference>>;
struct SonicAnimationTableResult {
    PlayerAnimationFailure failure=PlayerAnimationFailure::none;
    std::uint32_t source_boundary=0;
    std::size_t completed_records=0;
    explicit operator bool()const noexcept{return failure==PlayerAnimationFailure::none;}
};
// Actual7cec90,147 records. Caller owns the resolved ___SONIC_ACTIONS export;
// null/missing action entries and resource identity are retained independently.
SonicAnimationTableResult initialize_sonic_animation_table(PlayerAnimationTable&,
    const std::optional<std::shared_ptr<const SonicAnimationExport>>&);
}
