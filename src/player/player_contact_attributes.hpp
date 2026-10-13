#pragma once
#include "player/player_task.hpp"
#include "player/player_acceleration.hpp"

namespace sadx {
struct ContactParameterDefaults {
    std::optional<float> weight,air_resist_y,run_accel;
};
using ContactParameterTable=std::array<ContactParameterDefaults,256>;
struct PlayerContactAttributeConstants {
    float half,ordinary_weight_factor,air_y_factor,run_factor,alternate_weight_factor;
    std::array<float,5> friction; // source immediate stores: bit3,bit2,bit4,bit7,default
};
std::optional<PlayerContactAttributeConstants> decode_contact_attribute_constants(const std::uint8_t*,std::size_t);
enum class ContactAttributeFailure { none,unavailable_source_parameter,null_motion };
struct ContactAttributeResult {
    ContactAttributeFailure failure=ContactAttributeFailure::none;
    std::uint32_t source_boundary=0;
    explicit operator bool()const noexcept{return failure==ContactAttributeFailure::none;}
};
// Actual43cb90: OR source material bits, restore/modify the live parameter
// copy from source defaults, then overwrite motion friction. Shadow top flags
// select the numeric branch; friction priority uses the incoming LOW BYTE.
// Only source-known default fields are used. Missing fields retain all earlier
// stores and report their original read boundary; no invalid-row clamp exists.
ContactAttributeResult apply_player_contact_attributes(const PlayerTaskState&,PlayerMotionState*,
    PlayerPhysicsState&,PlayerParameters& live,const ContactParameterTable&,
    std::uint32_t incoming,const PlayerContactAttributeConstants&,X87Context&);
} // namespace sadx
