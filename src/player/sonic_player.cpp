#include "player/sonic_player.hpp"

namespace sadx::sonic {
Body body(std::uint8_t mode) {
    switch (static_cast<Mode>(mode)) {
    case Mode::Initialize:
        return Body::Initialize;
    case Mode::Stand:
        return Body::Stand;
    case Mode::Walk:
        return Body::Walk;
    case Mode::Hold:
    case Mode::PetLoop:
    case Mode::PetEnd:
    case Mode::Whistle:
    case Mode::SuperStand:
        return Body::Ground;
    case Mode::Spring:
    case Mode::HoldSpring:
    case Mode::SuperSpring:
        return Body::Air;
    case Mode::Parachute:
        return Body::GravityAir;
    case Mode::PlaceOn:
    case Mode::PanelWait:
    case Mode::Cart:
    case Mode::SnowWait:
    case Mode::IceFastSlide:
        return Body::CommonTail;
    default:
        return mode >= 88 ? Body::CommonTail : Body::Other;
    }
}
} // namespace sadx::sonic
