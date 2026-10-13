#include "player/player_contact_initializer.hpp"

#include <cstring>
#include <stdexcept>

namespace sadx {
float PlayerContactInitializationConstants::at(std::uint32_t address) const {
    for(std::size_t i=0;i<addresses.size();++i)if(addresses[i]==address){
        float value;std::memcpy(&value,&bits[i],4);return value;
    }
    throw std::out_of_range("Unknown source contact initialization constant");
}

PlayerContactInitializationResult initialize_player_contact_offset(
    NinjaVector& offset,const PlayerMotionState& world,const PlayerPhysicsState& player,
    const PlayerContactWork& contact,const PlayerParameters& p,
    std::optional<std::uint16_t> flags,const std::optional<NinjaVector>& gravity,
    const PlayerContactInitializationConstants& c,const PlayerRotationConstants& rotation,X87Context& fp) {
    offset.z=0;offset.x=0;
    const auto failed=[](PlayerContactInitializationFailure why,std::uint32_t site){
        return PlayerContactInitializationResult{why,site};};
    if(!player.prior_attributes)return failed(PlayerContactInitializationFailure::unavailable_prior_attributes,0x43c7e9);
    const auto attributes=*player.prior_attributes;
    const auto load=[&](float v){return fp.from_float(v);};
    const auto cmp=[&](float a,float b){return fp.compare_float_memory(load(a),b);};
    // JP after TEST AH,41 means greater OR unordered, unlike a C++ '>':
    const auto parity41=[](X87Comparison v){return v==X87Comparison::greater || v==X87Comparison::unordered;};
    const auto subtract_scaled=[&](std::uint32_t constant){
        const auto amount=fp.multiply_float_memory(load(p.pos_error),c.at(constant));
        // Most source paths FLD offset.y then FSUB ST1; both are registers.
        offset.y=fp.to_float(fp.subtract(load(offset.y),amount));
    };
    const auto subtract_full=[&](){offset.y=fp.to_float(fp.subtract(load(offset.y),load(p.pos_error)));};
    const auto zero=c.at(0x7dcb2c);
    if(attributes&0x4000U) {
        if(!parity41(cmp(world.motion.speed.y,zero))) {
            offset.y=fp.to_float(fp.reverse_subtract_float_memory(
                fp.multiply_float_memory(load(p.pos_error),c.at(0x7dd584)),offset.y));return {};
        }
        if(cmp(world.motion.speed.y,zero)==X87Comparison::greater) {
            offset.y=fp.to_float(fp.reverse_subtract_float_memory(
                fp.multiply_float_memory(load(p.pos_error),c.at(0x7dd58c)),offset.y));return {};
        }
    }
    if(attributes&0x20000U) {
        offset.y=fp.to_float(fp.subtract_float_memory(load(offset.y),p.pos_error));return {};
    }
    if(parity41(cmp(player.motion.speed.y,zero))) {
        if(!flags)return failed(PlayerContactInitializationFailure::unavailable_prior_flags,0x43ca1e);
        if(*flags&0x2000U)offset.y=fp.to_float(fp.subtract_float_memory(load(offset.y),p.pos_error));
        return {};
    }
    if(attributes&8U){subtract_scaled(0x7dd540);return {};}
    if(!gravity)return failed(PlayerContactInitializationFailure::unavailable_gravity,0x43c895);
    if(!contact.direction_at_1c)return failed(PlayerContactInitializationFailure::unavailable_direction,0x43c895);
    const auto dot=inner_product_extended(*gravity,*contact.direction_at_1c,fp).negated();
    if(!flags)return failed(PlayerContactInitializationFailure::unavailable_prior_flags,0x43c89e);
    if(*flags&0x2000U) {
        if(!parity41(fp.compare_float_memory(dot,c.at(0x7e0078))))subtract_scaled(0x7e0074);
        else if(!parity41(fp.compare_float_memory(dot,c.at(0x7e0070))))subtract_scaled(0x7e006c);
        else subtract_scaled(0x7e0068);
        return {};
    }
    const auto dash=cmp(player.motion.speed.x,p.dash_speed);
    if(dash!=X87Comparison::less && dash!=X87Comparison::unordered && (*flags&3U)) {
        subtract_full();return {};
    }
    if(!parity41(fp.compare_float_memory(dot,c.at(0x7e0064)))){subtract_full();return {};}
    if(fp.compare_float_memory(dot,c.at(0x7dd58c))==X87Comparison::less) {
        auto velocity=world.motion.speed;
        const auto length=rotation_vector_length(velocity,fp);
        const auto z=[&](){
            if(fp.compare_float_memory(length,zero)==X87Comparison::equal)return load(zero);
            (void)normalize_vector_extended(velocity,{rotation.single(0x7dcb2c),rotation.single(0x7dcb10)},fp);return load(velocity.z);
        }();
        const auto normalized=fp.compare_float_memory(z.negative()?z.negated():z,c.at(0x7dff54));
        subtract_scaled(normalized==X87Comparison::less?0x7e0060:0x7dd540);return {};
    }
    if(!parity41(cmp(player.motion.speed.x,p.run_speed))) {subtract_scaled(0x7dcc98);return {};}
    if(*flags&3U) {
        const auto ratio=fp.divide_float_memory(load(player.motion.speed.x),p.dash_speed);
        const auto amount=fp.multiply_float_memory(ratio,p.pos_error);
        offset.y=fp.to_float(fp.subtract(load(offset.y),amount));return {};
    }
    subtract_full();return {};
}
} // namespace sadx
