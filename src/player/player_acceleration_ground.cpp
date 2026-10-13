#include "player/player_acceleration_ground.hpp"
#include "player/player_acceleration_friction.hpp"

#include <cstring>
#include <stdexcept>

namespace sadx {
float GroundAccelerationConstants::at(std::uint32_t address) const {
    for(std::size_t i=0;i<addresses.size();++i) if(addresses[i]==address) {
        float value;std::memcpy(&value,&bits[i],4);return value;
    }
    throw std::logic_error("Unbound ground-acceleration source constant address");
}
PlayerAccelerationResult get_player_acceleration_ground(
    PlayerTaskState& task,PlayerMotionState& world,PlayerPhysicsState& player,
    const PlayerParameters& p,const GroundAccelerationConstants& constants,
    const PlayerAccelerationRuntime& runtime,const GroundAccelerationCallees& callees,X87Context& fp) {
    NinjaVector gravity{};
    bool gravity_available=false;
    const auto failure=[&](PlayerAccelerationFailure kind,std::uint32_t site,std::uint32_t callee=0){
        return PlayerAccelerationResult{kind,site,callee,
            gravity_available ? std::optional<NinjaVector>(gravity) : std::nullopt};
    };
    const auto mutation=[&](const GroundAccelerationMutation& fn,std::uint32_t site,std::uint32_t target){
        if(!fn)return failure(PlayerAccelerationFailure::missing_callee,site,target);
        const auto result=fn(task,world,player,p,fp);
        if(result)return PlayerAccelerationResult{};
        auto failed=result;failed.callee_failure=result.failure;
        failed.failure=PlayerAccelerationFailure::callee_unavailable;
        failed.source_boundary=site;failed.callee_address=target;
        failed.nested_source_boundary=result.source_boundary;return failed;
    };
    if((task.orientation.flags & 0x4000U)!=0) return mutation(callees.event_acceleration,0x44c293,0x447510);
    if((player.attributes & 0x20000U)!=0) return mutation(callees.attribute_20000_acceleration,0x44c2b8,0x44b0e0);
    if((player.attributes & 0x4000U)!=0) return mutation(callees.attribute_4000_acceleration,0x44c2d2,0x447010);
    if(!runtime.world_gravity)return failure(PlayerAccelerationFailure::unavailable_gravity,0x44c2e0);

    const auto load=[&](float value){return fp.from_float(value);};
    const auto c=[&](std::uint32_t address){return constants.at(address);};
    const auto cm=[&](X87Value a,float b){return fp.compare_float_memory(a,b);};
    const auto cf=[&](float a,float b){return cm(load(a),b);};
    const auto greater=[](X87Comparison value){return value==X87Comparison::greater;};
    const auto less=[](X87Comparison value){return value==X87Comparison::less;};
    const auto equal=[](X87Comparison value){return value==X87Comparison::equal;};
    // TEST AH masks include unordered behavior; JP AH&0x41 is greater OR
    // unordered, and must not become a host comparison or ordered '>='.
    const auto parity=[](X87Comparison value,unsigned mask){
        auto word=static_cast<unsigned>(x87_comparison_status(value)>>8U)&mask;
        unsigned ones=0;while(word){ones+=word&1U;word>>=1U;}return (ones&1U)==0;
    };
    const auto mul=[&](X87Value a,float b){return fp.multiply_float_memory(a,b);};
    const auto product=[&](float a,float b){return mul(load(a),b);};
    const auto stored_product=[&](float a,float b){return fp.to_float(product(a,b));};
    const auto update=[&](float& dst,X87Value value){dst=fp.to_float(value);};
    const auto zero=c(0x7dcb2c);
    gravity.x=fp.to_float(fp.add_float_memory(product(runtime.world_gravity->x,p.weight),world.motion.acceleration.x));
    gravity.y=fp.to_float(fp.add_float_memory(product(runtime.world_gravity->y,p.weight),world.motion.acceleration.y));
    gravity.z=fp.to_float(fp.add_float_memory(product(runtime.world_gravity->z,p.weight),world.motion.acceleration.z));
    gravity_available=true;
    if(!callees.world_to_player)return failure(PlayerAccelerationFailure::missing_world_to_player,0x44c357,0x43ec00);
    const auto mapped=callees.world_to_player(task.orientation,gravity,fp);
    if(!mapped){auto result=failure(PlayerAccelerationFailure::coordinate_unavailable,0x44c357,0x43ec00);
        result.coordinate_failure=mapped.failure;result.nested_source_boundary=mapped.source_boundary;return result;}

    std::int32_t angle;float magnitude;
    const auto input_task=input_task_view(task);
    const auto input=get_player_input(input_task,runtime.input,fp,&angle,&magnitude,runtime.forced_angle);
    if(!input || !input.active){auto result=failure(PlayerAccelerationFailure::input_unavailable,0x44c367,0x43bdc0);
        result.input_failure=input.failure;result.nested_source_boundary=input.source_boundary;return result;}
    const bool active=*input.active;
    const auto direction=player_contact_direction(player);
    if(!direction)return failure(PlayerAccelerationFailure::unavailable_collision_vector,0x44c36c,0x7889f0);
    NinjaVector cross;
    (void)cross_product_length_extended(*direction,world.motion.speed,cross,fp);
    auto retained_z=load(gravity.z);
    const auto fabs32=[](float value){std::uint32_t bits;std::memcpy(&bits,&value,4);bits&=0x7fffffffU;std::memcpy(&value,&bits,4);return value;};
    if(less(cf(player.dotp,c(0x7dd568))) && greater(cf(fabs32(cross.y),c(0x7dd584))) &&
       greater(cf(player.motion.speed.x,c(0x7e0178)))) {
        gravity.x=0;gravity.y=fp.to_float(load(p.weight).negated());retained_z=load(zero);
    } else if(less(cf(player.dotp,c(0x7e00e4))) && greater(cf(player.motion.speed.x,c(0x7e0178)))) {
        update(gravity.y,fp.reverse_subtract_float_memory(product(p.weight,c(0x7dccfc)),gravity.y));
    } else if(less(cf(player.dotp,c(0x7e01a8))) && greater(cf(player.motion.speed.x,c(0x7e0178)))) {
        update(gravity.y,fp.reverse_subtract_float_memory(product(p.weight,c(0x7dce5c)),gravity.y));
    } else if(less(cf(player.dotp,c(0x7e0064))) && greater(cf(player.motion.speed.x,c(0x7e0178)))) {
        update(gravity.y,fp.reverse_subtract_float_memory(product(p.weight,c(0x7dd550)),gravity.y));
    } else if(less(cf(player.dotp,c(0x7dcc98))) && less(cf(player.motion.speed.x,p.run_speed)) &&
              less(cm(load(p.run_speed).negated(),player.motion.speed.x))) {
        gravity.x=stored_product(gravity.x,c(0x7e01a4));retained_z=product(gravity.z,c(0x7e01a4));
    } else if(less(cf(player.dotp,c(0x7dd578))) && less(cf(player.motion.speed.x,p.run_speed)) &&
              less(cm(load(p.run_speed).negated(),player.motion.speed.x))) {
        retained_z=fp.add(load(gravity.z),load(gravity.z));
    } else if(less(cf(player.dotp,c(0x7e0124))) && less(cf(player.motion.speed.x,p.jog_speed)) &&
              less(cm(load(p.run_speed).negated(),player.motion.speed.x))) {
        retained_z=product(gravity.z,c(0x7e0068));
    }

    // Longitudinal drag: instructions may compare an unrounded intermediate
    // and always store the resulting acceleration separately.
    if(active && greater(cf(player.motion.speed.x,p.max_x_spd)) && greater(cf(player.dotp,c(0x7e0088)))) {
        update(gravity.x,fp.add_float_memory(mul(mul(fp.subtract_float_memory(load(player.motion.speed.x),p.max_x_spd),p.air_resist),c(0x7e019c)),gravity.x));
    } else if(!active && greater(cf(player.motion.speed.x,p.run_speed))) {
        update(gravity.x,fp.add_float_memory(product(p.air_resist,player.motion.speed.x),gravity.x));
    } else if(greater(cf(player.motion.speed.x,p.max_x_spd))) {
        update(gravity.x,fp.add_float_memory(mul(fp.subtract_float_memory(load(player.motion.speed.x),p.max_x_spd),p.air_resist),gravity.x));
    } else if(less(cf(player.motion.speed.x,zero))) {
        update(gravity.x,fp.add_float_memory(product(p.air_resist,player.motion.speed.x),gravity.x));
    }
    update(gravity.y,fp.add_float_memory(product(p.air_resist_y,player.motion.speed.y),gravity.y));
    update(gravity.z,fp.add(product(p.air_resist_z,player.motion.speed.z),retained_z));

    float drive=0,extra_y=0;
    if(!active) {
        if(less(cf(player.dotp,c(0x7e0138)))) {
            const auto result=mutation(callees.passive_rotation,0x44c620,0x4491e0);if(!result)return result;
        } else if(greater(cf(player.motion.speed.x,zero))) drive=p.slow_down;
        else if(less(cf(player.motion.speed.x,zero))) drive=fp.to_float(load(p.slow_down).negated());
        world.source_field_34=0;
    } else {
        if((task.orientation.flags & 0x2000U)!=0) {
            const auto result=mutation(callees.direction_reset,0x44c691,0x43c130);if(!result)return result;
            angle=world.target_yaw;
            // MOV literal 0xbf4ccccd, independent of source-constant lookup.
            const std::uint32_t literal=0xbf4ccccd;std::memcpy(&extra_y,&literal,4);
        } else if(less(cf(player.dotp,c(0x7e01a0))) && (player.attributes & 0x800000U)!=0) {
            const auto result=mutation(callees.attribute_rotation,0x44c6c5,0x449380);if(!result)return result;
        }

        X87Value gain=load(magnitude);bool store_gain=true;
        if(less(cf(player.motion.speed.x,p.max_x_spd))) {
            bool below_threshold=false;
            if(less(cf(player.motion.speed.x,p.jog_speed))) {
                if(greater(cf(magnitude,c(0x7dcc98)))) gain=product(magnitude,p.run_accel);
                else below_threshold=greater(cm(product(p.jog_speed,c(0x7dd550)),player.motion.speed.x));
                if(!below_threshold && !greater(cf(magnitude,c(0x7dcc98)))) {magnitude=0;store_gain=false;}
                else if(below_threshold) gain=product(magnitude,p.run_accel);
            } else if(less(cf(player.motion.speed.x,p.run_speed))) {
                if(greater(cf(magnitude,c(0x7dd578)))) gain=product(magnitude,p.run_accel);
                else {
                    const auto threshold=mul(fp.add(load(p.run_speed),load(p.run_speed)),c(0x7dcc98));
                    if(greater(cm(threshold,player.motion.speed.x))) gain=product(magnitude,p.run_accel);
                    else {magnitude=0;store_gain=false;}
                }
            } else if(less(cf(player.motion.speed.x,p.rush_speed))) {
                gain=product(magnitude,p.run_accel);
                if(!greater(cf(magnitude,c(0x7dfff0))))gain=mul(gain,c(0x7dd540));
            } else gain=product(magnitude,p.run_accel);
        } else {
            gain=product(magnitude,p.run_accel);
            const auto speed_comparison=cf(player.motion.speed.x,p.max_x_spd);
            if(speed_comparison==X87Comparison::less || speed_comparison==X87Comparison::unordered ||
               !less(cf(player.dotp,zero))) gain=mul(gain,c(0x7dd550));
        }
        if(store_gain)magnitude=fp.to_float(gain);
        const auto distance=angle_distance(task.orientation.angles[1],angle);
        const GroundYawMutation* yaw=&callees.yaw_regular;std::uint32_t site=0x44c904,target=0x443c50;
        if(equal(cf(player.motion.speed.x,zero)) && distance>0x1000) {
            world.target_yaw=angle;magnitude=0;yaw=&callees.yaw_fast;site=0x44c81e;target=0x443df0;
        } else if(!parity(cm(mul(fp.add_float_memory(load(p.run_speed),p.jog_speed),c(0x7dcc98)),player.motion.speed.x),0x41) && distance>0x1000) {
            world.target_yaw=angle;magnitude=p.slow_down;
        } else if((cf(player.motion.speed.x,p.jog_speed)!=X87Comparison::less &&
                   cf(player.motion.speed.x,p.jog_speed)!=X87Comparison::unordered && distance<0x1000) ||
                  (cf(player.motion.speed.x,p.dash_speed)!=X87Comparison::less &&
                   cf(player.motion.speed.x,p.dash_speed)!=X87Comparison::unordered && (player.attributes&0x800000U)!=0)) {
            world.target_yaw=angle;yaw=&callees.yaw_blended;
            site=distance<0x1000 ? 0x44c889 : 0x44c8b8;target=0x443e60;
        } else {
            if(cf(player.motion.speed.x,p.jog_speed)!=X87Comparison::less &&
               cf(player.motion.speed.x,p.jog_speed)!=X87Comparison::unordered &&
               !parity(cf(player.motion.speed.x,p.rush_speed),0x41) && distance>0x2000)
                magnitude=stored_product(magnitude,c(0x7dce5c));
            world.target_yaw=angle;
        }
        if(!*yaw)return failure(PlayerAccelerationFailure::missing_callee,site,target);
        const auto yaw_result=(*yaw)(task,world,player,p,angle,fp);
        if(!yaw_result) {
            auto failed=yaw_result;failed.callee_failure=yaw_result.failure;
            failed.failure=PlayerAccelerationFailure::callee_unavailable;
            failed.source_boundary=site;failed.callee_address=target;
            failed.nested_source_boundary=yaw_result.source_boundary;return failed;
        }
        world.source_field_34=magnitude;drive=magnitude;
    }

    auto friction_mode=LongitudinalFriction::apply;
    if(less(cf(player.dotp,c(0x7e0138))) && less(cf(player.motion.speed.x,p.jog_speed)) &&
       less(cm(load(p.jog_speed).negated(),player.motion.speed.x)) && !active) {
        gravity.x=stored_product(gravity.x,c(0x7dcb34));
        gravity.z=stored_product(gravity.z,c(0x7dcb34));
        friction_mode=LongitudinalFriction::skip;
    }
    finish_player_ground_friction(gravity,drive,extra_y,active,world,player,p,constants,friction_mode,fp);
    return {PlayerAccelerationFailure::none,0,0,gravity};
}
} // namespace sadx
