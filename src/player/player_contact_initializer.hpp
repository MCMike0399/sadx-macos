#pragma once

#include "player/player_contact.hpp"
#include "player/player_rotation.hpp"

namespace sadx {

struct PlayerContactInitializationConstants {
    static constexpr std::array<std::uint32_t,13> addresses{{
        0x7dcb2c,0x7dd584,0x7dd58c,0x7dd540,0x7e0078,0x7e0074,
        0x7e0070,0x7e006c,0x7e0068,0x7e0064,0x7dff54,0x7e0060,0x7dcc98}};
    std::array<std::uint32_t,13> bits;
    float at(std::uint32_t address) const;
};
enum class PlayerContactInitializationFailure {
    none, unavailable_prior_attributes, unavailable_prior_flags,
    unavailable_gravity, unavailable_direction,
};
struct PlayerContactInitializationResult {
    PlayerContactInitializationFailure failure = PlayerContactInitializationFailure::none;
    std::uint32_t source_boundary = 0;
    explicit operator bool() const noexcept { return failure==PlayerContactInitializationFailure::none; }
};

// Full source43c7d0..43ca3c: changes the local collision-center offset, with
// exact branch predicates and retained ext80 values. X/Z zero stores precede
// any later unavailable dependency; no rollback hides those mutations.
PlayerContactInitializationResult initialize_player_contact_offset(
    NinjaVector& offset,const PlayerMotionState&,const PlayerPhysicsState&,
    const PlayerContactWork&,const PlayerParameters&,
    std::optional<std::uint16_t> prior_flags,const std::optional<NinjaVector>& gravity,
    const PlayerContactInitializationConstants&,const PlayerRotationConstants&,X87Context&);

} // namespace sadx
