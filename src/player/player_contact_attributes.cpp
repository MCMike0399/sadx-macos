#include "player/player_contact_attributes.hpp"
#include <cstring>
namespace sadx {
std::optional<PlayerContactAttributeConstants> decode_contact_attribute_constants(const std::uint8_t* bytes,std::size_t size){
    if(!bytes||size!=40)return std::nullopt;
    PlayerContactAttributeConstants c;
    std::memcpy(&c.half,bytes,4);std::memcpy(&c.ordinary_weight_factor,bytes+4,4);
    std::memcpy(&c.air_y_factor,bytes+8,4);std::memcpy(&c.run_factor,bytes+12,4);
    std::memcpy(&c.alternate_weight_factor,bytes+16,4);std::memcpy(c.friction.data(),bytes+20,20);
    return c;
}
ContactAttributeResult apply_player_contact_attributes(const PlayerTaskState& task,PlayerMotionState* world,
    PlayerPhysicsState& player,PlayerParameters& live,const ContactParameterTable& defaults,
    std::uint32_t incoming,const PlayerContactAttributeConstants& c,X87Context& fp){
    player.attributes|=incoming; // first original store, even at a later missing owner
    const auto& row=defaults[task.character_id];
    if(player.shadow_attributes_top&2U){
        if(!row.weight)return {ContactAttributeFailure::unavailable_source_parameter,task.character_id==5?0x43cba6U:0x43cbbdU};
        live.weight=fp.to_float(fp.multiply_float_memory(fp.from_float(*row.weight),task.character_id==5?c.half:c.ordinary_weight_factor));
        if(!row.air_resist_y)return {ContactAttributeFailure::unavailable_source_parameter,0x43cbd9};
        live.air_resist_y=fp.to_float(fp.multiply_float_memory(fp.from_float(*row.air_resist_y),c.air_y_factor));
        if(!row.run_accel)return {ContactAttributeFailure::unavailable_source_parameter,0x43cbf5};
        live.run_accel=fp.to_float(fp.multiply_float_memory(fp.from_float(*row.run_accel),c.run_factor));
    }else if(player.shadow_attributes_top&0x400000U){
        if(!row.weight)return {ContactAttributeFailure::unavailable_source_parameter,0x43cc1e};
        live.weight=fp.to_float(fp.multiply_float_memory(fp.from_float(*row.weight),c.alternate_weight_factor));
        if(!row.air_resist_y)return {ContactAttributeFailure::unavailable_source_parameter,0x43cc3a};
        const auto y=fp.from_float(*row.air_resist_y);live.air_resist_y=fp.to_float(fp.add(y,y));
        if(!row.run_accel)return {ContactAttributeFailure::unavailable_source_parameter,0x43cc52};
        live.run_accel=fp.to_float(fp.multiply_float_memory(fp.from_float(*row.run_accel),c.ordinary_weight_factor));
    }else{
        // MOVs preserve signaling encodings and do not accrue FP flags.
        if(!row.weight)return {ContactAttributeFailure::unavailable_source_parameter,0x43cc6c};
        live.weight=*row.weight;
        if(!row.air_resist_y)return {ContactAttributeFailure::unavailable_source_parameter,0x43cc82};
        live.air_resist_y=*row.air_resist_y;
        if(!row.run_accel)return {ContactAttributeFailure::unavailable_source_parameter,0x43cc98};
        live.run_accel=*row.run_accel;
    }
    const auto low=static_cast<std::uint8_t>(incoming);
    const std::size_t selected=(low&8)?0:(low&4)?1:(low&0x10)?2:(low&0x80)?3:4;
    if(!world){
        constexpr std::uint32_t boundaries[]={0x43cca9,0x43ccb6,0x43ccc3,0x43cccf,0x43ccd7};
        return {ContactAttributeFailure::null_motion,boundaries[selected]};
    }
    world->friction_scale=c.friction[selected];return {};
}
} // namespace sadx
