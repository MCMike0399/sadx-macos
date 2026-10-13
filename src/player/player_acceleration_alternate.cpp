#include "player/player_acceleration_alternate.hpp"
#include "player/player_acceleration_alternate_math.hpp"
#include "player/player_rotation.hpp"
#include "player/player_acceleration_friction.hpp"
#include "cpu/crt_integer.hpp"
#include "cpu/x87_transcendental.hpp"
#include <cstring>

namespace sadx {
namespace {
PlayerAccelerationResult failed_callee(PlayerAccelerationResult child,std::uint32_t site,
    std::uint32_t target,const std::optional<NinjaVector>& caller_temporary,
    const std::optional<FloorProjectionResult>& floor=std::nullopt,
    const std::optional<ProjectionAngleResult>& projection=std::nullopt) {
    child.callee_failure=child.failure;child.failure=PlayerAccelerationFailure::callee_unavailable;
    child.nested_source_boundary=child.source_boundary;child.source_boundary=site;child.callee_address=target;
    if(!child.temporary_acceleration)child.temporary_acceleration=caller_temporary;
    if(!child.floor_projection)child.floor_projection=floor;
    if(!child.projection)child.projection=projection;
    return child;
}
}

PlayerAccelerationResult get_player_acceleration_attribute_4000(
    PlayerTaskState& task,PlayerMotionState& world,PlayerPhysicsState& player,
    const PlayerParameters& p,const GroundAccelerationConstants& c,
    const PlayerAccelerationRuntime& runtime,const GroundAccelerationCallees& callees,X87Context& fp) {
    std::optional<NinjaVector> temporary;
    std::optional<PlayerRotationResult> rotation_facts;
    const auto failed=[&](PlayerAccelerationFailure failure,std::uint32_t site,std::uint32_t target=0){
        return PlayerAccelerationResult{failure,site,target,temporary};};
    if(!runtime.world_gravity)return failed(PlayerAccelerationFailure::unavailable_gravity,0x447013);
    const auto load=[&](float value){return fp.from_float(value);};
    const auto cmp=[&](float a,float b){return fp.compare_float_memory(load(a),b);};
    const auto cm=[&](X87Value a,float b){return fp.compare_float_memory(a,b);};
    const auto less=[](X87Comparison value){return value==X87Comparison::less;};
    const auto greater=[](X87Comparison value){return value==X87Comparison::greater;};
    const auto equal=[](X87Comparison value){return value==X87Comparison::equal;};
    const auto mul=[&](float a,float b){return fp.multiply_float_memory(load(a),b);};
    const auto multiply=[&](X87Value a,float b){return fp.multiply_float_memory(a,b);};
    const auto add=[&](X87Value a,float b){return fp.add_float_memory(a,b);};
    const auto store=[&](float& field,X87Value value){field=fp.to_float(value);};
    const auto zero=c.at(0x7dcb2c);
    NinjaVector g;
    g.x=fp.to_float(add(mul(runtime.world_gravity->x,p.weight),world.motion.acceleration.x));
    g.y=fp.to_float(add(mul(runtime.world_gravity->y,p.weight),world.motion.acceleration.y));
    g.z=fp.to_float(add(mul(runtime.world_gravity->z,p.weight),world.motion.acceleration.z));temporary=g;
    std::int32_t angle;float magnitude;
    const auto input=get_player_input(input_task_view(task),runtime.input,fp,&angle,&magnitude,runtime.forced_angle);
    if(!input || !input.active) {
        auto result=failed(PlayerAccelerationFailure::input_unavailable,0x447090,0x43bdc0);
        result.input_failure=input.failure;result.nested_source_boundary=input.source_boundary;return result;
    }
    const bool active=*input.active;
    if(!active && greater(cmp(player.motion.speed.x,p.run_speed))) {
        store(g.x,add(mul(p.air_resist,player.motion.speed.x),g.x));
    } else if(greater(cmp(player.motion.speed.x,p.rush_speed))) {
        const auto difference=fp.subtract_float_memory(load(player.motion.speed.x),p.rush_speed);
        store(g.x,add(multiply(difference,p.air_resist),g.x));
    } else if(less(cmp(player.motion.speed.x,zero)))store(g.x,add(mul(p.air_resist,player.motion.speed.x),g.x));
    store(g.y,add(mul(p.air_resist_y,player.motion.speed.y),g.y));
    store(g.z,add(mul(p.air_resist_z,player.motion.speed.z),g.z));temporary=g;
    float drive=0,extra_z=0;
    if(!active) {
        if(greater(cmp(player.motion.speed.x,zero)))drive=p.slow_down;
        else if(less(cmp(player.motion.speed.x,zero)))drive=fp.to_float(load(p.slow_down).negated());
        world.source_field_34=0;
    } else {
        // The source saves this FCOM result, then multiplies before FNSTSW.
        const auto run_above_speed=cmp(p.run_speed,player.motion.speed.x);
        if((input.outputs_written&3U)!=3U)
            return failed(PlayerAccelerationFailure::input_outputs_unavailable,0x44716a,0x43bdc0);
        auto gain=mul(magnitude,p.run_accel);
        if(!greater(run_above_speed))gain=multiply(gain,c.at(0x7dd550));
        magnitude=fp.to_float(gain);
        const auto distance=angle_distance(task.orientation.angles[1],angle);
        const GroundYawMutation* yaw=&callees.yaw_regular;
        std::uint32_t site=0x44726a,target=0x443c50;
        if(equal(cmp(player.motion.speed.x,zero)) && distance>0x1000U) {
            world.target_yaw=angle;magnitude=0;yaw=&callees.yaw_fast;site=0x4471c2;target=0x443df0;
        } else {
            const auto jog=load(p.jog_speed);
            const auto midpoint=multiply(add(jog,p.run_speed),c.at(0x7dcc98));
            const auto midpoint_cmp=cm(midpoint,player.motion.speed.x);
            if((midpoint_cmp==X87Comparison::less || midpoint_cmp==X87Comparison::equal) && distance>0x1000U) {
                world.target_yaw=angle;magnitude=p.slow_down;
            } else {
                const auto jog_cmp=cmp(player.motion.speed.x,p.jog_speed);
                if(jog_cmp!=X87Comparison::less && jog_cmp!=X87Comparison::unordered && distance<0x1000U) {
                    world.target_yaw=angle;yaw=&callees.yaw_blended;site=0x447222;target=0x443e60;
                } else {
                    const auto rush_cmp=cmp(player.motion.speed.x,p.rush_speed);
                    if(jog_cmp!=X87Comparison::less && jog_cmp!=X87Comparison::unordered &&
                       (rush_cmp==X87Comparison::less || rush_cmp==X87Comparison::equal) && distance>0x2000U)
                        magnitude=fp.to_float(mul(magnitude,c.at(0x7dce5c)));
                    world.target_yaw=angle;
                }
            }
        }
        if(!*yaw)return failed(PlayerAccelerationFailure::missing_callee,site,target);
        const auto result=(*yaw)(task,world,player,p,angle,fp);
        if(!result)return failed_callee(result,site,target,temporary);
        rotation_facts=result.rotation;
        world.source_field_34=magnitude;drive=magnitude;
    }
    const auto friction=[&](float coefficient){return multiply(multiply(load(coefficient),world.friction_scale),g.y);};
    if(equal(cmp(player.motion.speed.x,zero))) {
        const float threshold=fp.to_float(friction(p.lim_frict));store(g.x,add(load(g.x),drive));
        if(!active && less(cmp(g.x,threshold)) && less(cm(load(threshold).negated(),g.x)))g.x=0;
    } else if(less(cmp(drive,zero))) {
        if(greater(cmp(player.motion.speed.x,zero))) {
            auto sum=add(load(g.x),drive);
            if(less(cm(fp.multiply(load(drive),sum),zero)))sum=load(zero);
            // Source FMUL uses the retained sum directly; the binary32 store
            // below is the sole final sum store (no intermediate FST).
            store(g.x,sum);
        } else {
            auto sum=add(add(friction(p.lim_frict),g.x),drive);
            const auto sign=cm(sum,zero);
            if(sign!=X87Comparison::less && sign!=X87Comparison::unordered)sum=load(zero);
            store(g.x,sum);
        }
    } else {
        const float threshold=fp.to_float(friction(p.lim_frict));
        if(less(cmp(player.motion.speed.x,zero))) {
            const auto sum=add(load(g.x),drive);g.x=fp.to_float(sum);
            if(less(cm(fp.multiply_float_memory(sum,drive),zero)))g.x=0;
        } else if(greater(cmp(threshold,zero)) && less(cmp(threshold,drive))) {
            auto sum=add(load(g.x),drive);
            if(greater(cm(sum,threshold))) {
                world.source_field_38=fp.to_float(fp.subtract_float_memory(sum,threshold));sum=load(threshold);
            }
            store(g.x,sum);
        } else store(g.x,add(load(g.x),drive));
    }
    if(equal(cmp(player.motion.speed.z,zero))) {
        const auto threshold=friction(p.lim_frict);
        if(less(fp.compare(load(g.z),threshold)) && less(cm(threshold.negated(),g.z)))g.z=0;
    } else {
        const auto threshold=less(cmp(g.y,zero)) ? friction(p.grd_frict_z) : load(zero);
        if(greater(cmp(g.z,zero)))extra_z=fp.to_float(threshold.negated());
        else if(less(cmp(g.z,zero)))extra_z=fp.to_float(threshold);
        auto sum=add(load(g.z),extra_z);
        if(!equal(cmp(g.z,zero)) && !equal(cmp(extra_z,zero)) && less(cm(fp.multiply(load(g.z),sum),zero)))sum=load(zero);
        store(g.z,sum);
    }
    player.motion.acceleration.x=g.x;player.motion.acceleration.y=g.y;player.motion.acceleration.z=g.z;
    auto result=PlayerAccelerationResult{PlayerAccelerationFailure::none,0,0,g};result.rotation=rotation_facts;return result;
}

namespace {
// Source event-ground friction has several deliberate differences from the
// attribute4000 body above; in particular it gates cleanup on drive equality,
// and its negative-drive friction path omits the prior X acceleration.
void finish_event_friction(NinjaVector& g,float drive,PlayerMotionState& world,
    PlayerPhysicsState& player,const PlayerParameters& p,const GroundAccelerationConstants& c,X87Context& fp) {
    const auto load=[&](float value){return fp.from_float(value);};
    const auto cmp=[&](float a,float b){return fp.compare_float_memory(load(a),b);};
    const auto cm=[&](X87Value a,float b){return fp.compare_float_memory(a,b);};
    const auto less=[](X87Comparison v){return v==X87Comparison::less;};
    const auto greater=[](X87Comparison v){return v==X87Comparison::greater;};
    const auto equal=[](X87Comparison v){return v==X87Comparison::equal;};
    const auto add=[&](X87Value a,float b){return fp.add_float_memory(a,b);};
    const auto friction=[&](float coefficient){return fp.multiply_float_memory(
        fp.multiply_float_memory(load(coefficient),world.friction_scale),g.y);};
    const auto small=[&](){return less(cmp(g.x,c.at(0x7e014c))) && greater(cmp(g.x,c.at(0x7e0148)));};
    const auto low_speed=[&](){auto v=cmp(player.motion.speed.x,p.jog_speed);return v==X87Comparison::less || v==X87Comparison::equal;};
    const auto zero=c.at(0x7dcb2c);
    if(equal(cmp(player.motion.speed.x,zero))) {
        const auto limit=fp.to_float(friction(p.lim_frict));g.x=fp.to_float(add(load(g.x),drive));
        if(equal(cmp(drive,zero))) {
            if(less(cmp(g.x,limit)) && less(cm(load(limit).negated(),g.x)))g.x=0;
            if(equal(cmp(drive,zero)) && small())g.x=0;
        }
    } else if(less(cmp(drive,zero))) {
        if(greater(cmp(player.motion.speed.x,zero))) {
            auto sum=add(load(g.x),drive);
            if(less(cm(fp.multiply(load(drive),sum),zero)))sum=load(zero);
            g.x=fp.to_float(sum);
        } else {
            auto sum=add(friction(p.lim_frict),drive);const auto sign=cm(sum,zero);
            if(sign!=X87Comparison::less && sign!=X87Comparison::unordered)sum=load(zero);
            g.x=fp.to_float(sum);
        }
    } else {
        const auto limit=fp.to_float(friction(p.lim_frict));
        if(less(cmp(player.motion.speed.x,zero)) && small()) {
            const auto sum=add(load(g.x),drive);g.x=fp.to_float(sum);
            if(less(cm(fp.multiply_float_memory(sum,drive),zero)))g.x=0;
        } else if(greater(cmp(limit,zero)) && less(cmp(limit,drive))) {
            auto sum=add(load(g.x),drive);
            if(greater(cm(sum,limit))) {
                world.source_field_38=fp.to_float(fp.subtract_float_memory(sum,limit));sum=load(limit);
            }
            g.x=fp.to_float(sum);
        } else if(equal(cmp(drive,zero)) && low_speed() && small())g.x=0;
        else g.x=fp.to_float(add(load(g.x),drive));
    }
    float extra_z=0;
    if(equal(cmp(player.motion.speed.z,zero))) {
        const auto limit=friction(p.lim_frict);
        if(less(fp.compare(load(g.z),limit)) && less(cm(limit.negated(),g.z)))g.z=0;
    } else {
        const auto limit=less(cmp(g.y,zero)) ? friction(p.grd_frict_z) : load(zero);
        if(greater(cmp(g.z,zero)))extra_z=fp.to_float(limit.negated());
        else if(less(cmp(g.z,zero)))extra_z=fp.to_float(limit);
        auto sum=add(load(g.z),extra_z);
        if(!equal(cmp(g.z,zero)) && !equal(cmp(extra_z,zero)) && less(cm(fp.multiply(load(g.z),sum),zero)))sum=load(zero);
        g.z=fp.to_float(sum);
    }
}

PlayerAccelerationResult event_acceleration(bool air,PlayerTaskState& task,PlayerMotionState& world,
    PlayerPhysicsState& player,const PlayerParameters& p,const GroundAccelerationConstants& c,
    const EventAccelerationConstants& ec,const PlayerAccelerationRuntime& runtime,
    const GroundAccelerationCallees& callees,X87Context& fp) {
    std::optional<NinjaVector> temporary;
    const auto fail=[&](PlayerAccelerationFailure why,std::uint32_t ground_site,std::uint32_t air_site,std::uint32_t target=0){
        return PlayerAccelerationResult{why,air?air_site:ground_site,target,temporary};};
    if(!runtime.world_gravity)return fail(PlayerAccelerationFailure::unavailable_gravity,0x447513,0x447b93);
    const auto load=[&](float value){return fp.from_float(value);};
    const auto multiply=[&](float a,float b){return fp.multiply_float_memory(load(a),b);};
    const auto add=[&](X87Value a,float b){return fp.add_float_memory(a,b);};
    const auto cmp=[&](X87Value a,float b){return fp.compare_float_memory(a,b);};
    const auto less=[](X87Comparison v){return v==X87Comparison::less;};
    const auto greater=[](X87Comparison v){return v==X87Comparison::greater;};
    const auto equal=[](X87Comparison v){return v==X87Comparison::equal;};
    const auto zero=c.at(0x7dcb2c);
    NinjaVector g;
    g.x=fp.to_float(add(multiply(runtime.world_gravity->x,p.weight),world.motion.acceleration.x));
    g.y=fp.to_float(add(multiply(runtime.world_gravity->y,p.weight),world.motion.acceleration.y));
    g.z=fp.to_float(add(multiply(runtime.world_gravity->z,p.weight),world.motion.acceleration.z));temporary=g;
    if(!task.event_move || !task.event_move->mode)
        return fail(PlayerAccelerationFailure::unavailable_event_mode,0x447589,0x447c01);
    auto& event=*task.event_move;const auto mode=*event.mode;float drive=0;
    if(mode<1 || mode>4)task.orientation.flags&=static_cast<std::uint16_t>(~0x4000U);
    else if(mode!=1) {
        if(!event.vector_at_28)return fail(PlayerAccelerationFailure::unavailable_event_vector,
            mode==2?0x44759e:mode==3?0x447684:0x447734,mode==2?0x447c16:mode==3?0x447cc0:0x447d6d);
        auto vector=*event.vector_at_28;
        if(mode==2) {
            if(!task.position)return fail(PlayerAccelerationFailure::unavailable_task_position,0x4475bc,0x447c34,0x787610);
            event_vector_subtract(vector,*task.position,fp);
        }
        vector.y=0;const auto length=rotation_vector_length(vector,fp);
        const auto length_cmp=cmp(length,mode==2 ? ec.target_stop_distance : zero);
        const bool stop=mode==2 ? length_cmp==X87Comparison::less || length_cmp==X87Comparison::equal : equal(length_cmp);
        if(stop) {
            player.motion.speed.z=0;player.motion.speed.y=0;player.motion.speed.x=0;event.mode=1;
        } else {
            world.target_yaw=static_cast<std::int32_t>(event_horizontal_heading(vector,ec.angle_units,ec.positive_factor,fp));
            if(mode==4)event_vector_add(g,vector,fp);
            else {
                X87Value desired=length;
                if(mode==2) {
                    if(air)desired=load(p.run_speed);
                    else {
                        if(!event.target_speed)return fail(PlayerAccelerationFailure::unavailable_event_target_speed,0x447616,0x447c7e);
                        desired=load(*event.target_speed);
                        if(equal(cmp(desired,zero)))desired=load(p.run_speed);
                    }
                }
                float step=p.run_accel;
                if(!air && mode==2) {
                    if(!event.acceleration_step)return fail(PlayerAccelerationFailure::unavailable_event_acceleration_step,0x44762e,0x447c7e);
                    const auto loaded=load(*event.acceleration_step);step=fp.to_float(loaded);
                    if(equal(cmp(loaded,zero)))step=p.run_accel;
                }
                const auto difference=fp.subtract_float_memory(desired,player.motion.speed.x);
                drive=fp.to_float(difference);
                if(greater(cmp(difference,zero))) {
                    if(greater(cmp(load(drive),step)))drive=step;
                } else drive=0;
            }
        }
    }
    temporary=g;
    if(!callees.world_to_player)return fail(PlayerAccelerationFailure::missing_world_to_player,0x4477b6,0x447de2,0x43ec00);
    const auto mapped=callees.world_to_player(task.orientation,g,fp);
    if(!mapped){auto result=fail(PlayerAccelerationFailure::coordinate_unavailable,0x4477b6,0x447de2,0x43ec00);
        result.coordinate_failure=mapped.failure;result.nested_source_boundary=mapped.source_boundary;return result;}
    if(air)g.x=fp.to_float(add(multiply(p.air_resist_air,player.motion.speed.x),g.x));
    else {
        if(greater(cmp(load(player.motion.speed.x),p.max_x_spd))) {
            const auto difference=fp.subtract_float_memory(load(player.motion.speed.x),p.max_x_spd);
            g.x=fp.to_float(add(fp.multiply_float_memory(difference,p.air_resist),g.x));
        } else if(less(cmp(load(player.motion.speed.x),zero)))g.x=fp.to_float(add(multiply(p.air_resist,player.motion.speed.x),g.x));
    }
    g.y=fp.to_float(add(multiply(p.air_resist_y,player.motion.speed.y),g.y));
    g.z=fp.to_float(add(multiply(p.air_resist_z,player.motion.speed.z),g.z));temporary=g;
    if(!callees.yaw_fast)return fail(PlayerAccelerationFailure::missing_callee,0x447826,0x447e21,0x443df0);
    const auto turned=callees.yaw_fast(task,world,player,p,world.target_yaw,fp);
    if(!turned)return failed_callee(turned,air?0x447e21:0x447826,0x443df0,temporary);
    if(air)g.x=fp.to_float(add(load(g.x),drive));
    else finish_event_friction(g,drive,world,player,p,c,fp);
    player.motion.acceleration.x=g.x;player.motion.acceleration.y=g.y;player.motion.acceleration.z=g.z;
    auto result=PlayerAccelerationResult{PlayerAccelerationFailure::none,0,0,g};
    result.rotation=turned.rotation;
    return result;
}
} // namespace

PlayerAccelerationResult get_player_acceleration_event_ground(PlayerTaskState& task,PlayerMotionState& world,
    PlayerPhysicsState& player,const PlayerParameters& p,const GroundAccelerationConstants& c,
    const EventAccelerationConstants& ec,const PlayerAccelerationRuntime& runtime,const GroundAccelerationCallees& callees,X87Context& fp) {
    return event_acceleration(false,task,world,player,p,c,ec,runtime,callees,fp);
}
PlayerAccelerationResult get_player_acceleration_event_air(PlayerTaskState& task,PlayerMotionState& world,
    PlayerPhysicsState& player,const PlayerParameters& p,const GroundAccelerationConstants& c,
    const EventAccelerationConstants& ec,const PlayerAccelerationRuntime& runtime,const GroundAccelerationCallees& callees,X87Context& fp) {
    return event_acceleration(true,task,world,player,p,c,ec,runtime,callees,fp);
}

PlayerAccelerationResult get_player_acceleration_attribute_20000(
    PlayerTaskState& task,PlayerMotionState& world,PlayerPhysicsState& player,
    const PlayerParameters& p,const GroundAccelerationConstants& constants,const EventAccelerationConstants& ec,
    const PlayerAccelerationRuntime& runtime,const AlternateSurfaceRuntime& surface,
    const GroundAccelerationCallees& callees,PlayerCoordinates& coordinates,const CoordinateAxisConstants& axis,
    const ProjectionAngleConstants& projection_constants,const CrtAsinRuntime& crt,X87Context& fp) {
    std::optional<NinjaVector> temporary;
    std::optional<FloorProjectionResult> projection_facts;
    std::optional<ProjectionAngleResult> angle_facts;
    std::optional<PlayerRotationResult> rotation_facts;
    const auto fail=[&](PlayerAccelerationFailure why,std::uint32_t site,std::uint32_t target=0){
        auto result=PlayerAccelerationResult{why,site,target,temporary};result.floor_projection=projection_facts;result.projection=angle_facts;result.rotation=rotation_facts;return result;};
    if(!player.floor_normal)return fail(PlayerAccelerationFailure::unavailable_floor_normal,0x44b0eb);
    const auto load=[&](float value){return fp.from_float(value);};
    const auto c=[&](std::uint32_t address){return constants.at(address);};
    const auto cm=[&](X87Value a,float b){return fp.compare_float_memory(a,b);};
    const auto cf=[&](float a,float b){return cm(load(a),b);};
    const auto less=[](X87Comparison v){return v==X87Comparison::less;};
    const auto greater=[](X87Comparison v){return v==X87Comparison::greater;};
    const auto equal=[](X87Comparison v){return v==X87Comparison::equal;};
    const auto mul=[&](X87Value a,float b){return fp.multiply_float_memory(a,b);};
    const auto product=[&](float a,float b){return mul(load(a),b);};
    const auto add=[&](X87Value a,float b){return fp.add_float_memory(a,b);};
    const auto update=[&](float& dst,X87Value value){dst=fp.to_float(value);};
    const auto zero=c(0x7dcb2c);NinjaVector g;
    g.x=fp.to_float(fp.reverse_subtract_float_memory(product(player.floor_normal->x,p.weight),world.motion.acceleration.x));
    g.y=fp.to_float(fp.reverse_subtract_float_memory(product(player.floor_normal->y,p.weight),world.motion.acceleration.y));
    g.z=fp.to_float(fp.reverse_subtract_float_memory(product(player.floor_normal->z,p.weight),world.motion.acceleration.z));temporary=g;
    const auto mapped=coordinates.world_to_player(task.orientation,g,fp);
    if(!mapped){auto result=fail(PlayerAccelerationFailure::coordinate_unavailable,0x44b156,0x43ec00);result.coordinate_failure=mapped.failure;result.nested_source_boundary=mapped.source_boundary;return result;}
    std::int32_t angle;float magnitude;
    const auto input=get_player_input(input_task_view(task),runtime.input,fp,&angle,&magnitude,runtime.forced_angle);
    if(!input || !input.active){auto result=fail(PlayerAccelerationFailure::input_unavailable,0x44b166,0x43bdc0);result.input_failure=input.failure;result.nested_source_boundary=input.source_boundary;return result;}
    const bool active=*input.active;
    if(active && greater(cf(player.motion.speed.x,p.max_x_spd)) && greater(cf(player.dotp,c(0x7e0088))))
        update(g.x,add(mul(mul(fp.subtract_float_memory(load(player.motion.speed.x),p.max_x_spd),p.air_resist),c(0x7e019c)),g.x));
    else if(!active && greater(cf(player.motion.speed.x,p.run_speed)))update(g.x,add(product(p.air_resist,player.motion.speed.x),g.x));
    else if(greater(cf(player.motion.speed.x,p.max_x_spd)))update(g.x,add(mul(fp.subtract_float_memory(load(player.motion.speed.x),p.max_x_spd),p.air_resist),g.x));
    else if(less(cf(player.motion.speed.x,zero)))update(g.x,add(product(p.air_resist,player.motion.speed.x),g.x));
    update(g.y,add(product(p.air_resist_y,player.motion.speed.y),g.y));
    update(g.z,add(product(p.air_resist_z,player.motion.speed.z),g.z));temporary=g;
    float drive=0,extra_y=0;
    if(!active) {
        if(less(cf(player.dotp,c(0x7e0138)))) {
            if(!callees.passive_rotation)return fail(PlayerAccelerationFailure::missing_callee,0x44b234,0x4491e0);
            const auto result=callees.passive_rotation(task,world,player,p,fp);
            if(!result)return failed_callee(result,0x44b234,0x4491e0,temporary,projection_facts,angle_facts);
            rotation_facts=result.rotation;
        } else if(greater(cf(player.motion.speed.x,zero)))drive=p.slow_down;
        else if(less(cf(player.motion.speed.x,zero)))drive=fp.to_float(load(p.slow_down).negated());
        world.source_field_34=0;
    } else {
        if((task.orientation.flags&0x2000U)!=0) {
            if(!callees.direction_reset)return fail(PlayerAccelerationFailure::missing_callee,0x44b2af,0x43c130);
            const auto result=callees.direction_reset(task,world,player,p,fp);
            if(!result)return failed_callee(result,0x44b2af,0x43c130,temporary,projection_facts,angle_facts);
            rotation_facts=result.rotation;
            angle=world.target_yaw;const std::uint32_t literal=0xbf4ccccdU;std::memcpy(&extra_y,&literal,4);
        } else if((input.outputs_written&1U)==0)return fail(PlayerAccelerationFailure::input_outputs_unavailable,0x44b2c1,0x43bdc0);
        if((input.outputs_written&2U)==0)return fail(PlayerAccelerationFailure::input_outputs_unavailable,0x44b2e9,0x43bdc0);
        auto gain=load(magnitude);bool store_gain=true;
        if(less(cf(player.motion.speed.x,p.max_x_spd))) {
            if(less(cf(player.motion.speed.x,p.jog_speed))) {
                if(greater(cf(magnitude,c(0x7dcc98))))gain=product(magnitude,p.run_accel);
                else if(greater(cm(product(p.jog_speed,c(0x7dd550)),player.motion.speed.x)))gain=product(magnitude,p.run_accel);
                else {magnitude=0;store_gain=false;}
            } else if(less(cf(player.motion.speed.x,p.run_speed))) {
                if(greater(cf(magnitude,c(0x7dd578))))gain=product(magnitude,p.run_accel);
                else if(greater(cm(mul(fp.add(load(p.run_speed),load(p.run_speed)),c(0x7dcc98)),player.motion.speed.x)))gain=product(magnitude,p.run_accel);
                else {magnitude=0;store_gain=false;}
            } else if(less(cf(player.motion.speed.x,p.rush_speed))) {
                gain=product(magnitude,p.run_accel);if(!greater(cf(magnitude,c(0x7dfff0))))gain=mul(gain,c(0x7dd540));
            } else gain=product(magnitude,p.run_accel);
        } else {
            gain=product(magnitude,p.run_accel);const auto cmp=cf(player.motion.speed.x,p.max_x_spd);
            if(cmp==X87Comparison::less || cmp==X87Comparison::unordered || !less(cf(player.dotp,zero)))gain=mul(gain,c(0x7dd550));
        }
        if(store_gain)magnitude=fp.to_float(gain);
        const auto distance=angle_distance(task.orientation.angles[1],angle);
        const auto retained_zero=load(zero);
        if(equal(cm(retained_zero,player.motion.speed.x)) && distance>0x1000U) {
            magnitude=fp.to_float(load(zero));world.target_yaw=angle;
            if(!callees.yaw_fast)return fail(PlayerAccelerationFailure::missing_callee,0x44b408,0x443df0);
            const auto result=callees.yaw_fast(task,world,player,p,angle,fp);
            if(!result)return failed_callee(result,0x44b408,0x443df0,temporary,projection_facts,angle_facts);
            if(result.rotation)rotation_facts=result.rotation;
        } else {
            if(task.player_index>=surface.raw_axes.size() || !surface.raw_axes[task.player_index])return fail(PlayerAccelerationFailure::unavailable_raw_axes,0x44b41c);
            const auto& raw=*surface.raw_axes[task.player_index];
            const auto loaded_x=fp.from_int64(static_cast<std::int64_t>(raw[0])*256);
            const auto loaded_y=fp.from_int64(static_cast<std::int64_t>(raw[1])*256);
            const auto atan=X87Transcendental::fpatan_bochs_p5_p6(fp,loaded_x,loaded_y);
            const auto scaled=fp.multiply_double_memory(atan,ec.angle_units);
            const auto converted=crt_ftol_low32(fp.multiply_double_memory(scaled,ec.positive_factor),fp);
            std::int32_t raw_angle;std::memcpy(&raw_angle,&converted,4);
            if(!surface.camera_pitch_yaw)return fail(PlayerAccelerationFailure::unavailable_camera,0x44b471);
            // Source FCOM of its loaded zero does not pop it. That register
            // survives raw-stick FPATAN/_ftol and feeds the two FST32 stores.
            const auto projected=coordinates.project_input_to_floor(task.orientation,*player.floor_normal,raw_angle,
                (*surface.camera_pitch_yaw)[0],(*surface.camera_pitch_yaw)[1],retained_zero,axis,fp);
            projection_facts=projected;
            if(!projected || !projected.projected_direction || !projected.forward_direction) {
                auto out=fail(PlayerAccelerationFailure::projection_unavailable,projected.source_boundary,0x788df0);out.coordinate_failure=projected.coordinate.failure;return out;
            }
            const auto signed_angle=projection_vector_angle(*projected.forward_direction,*projected.projected_direction,
                &*player.floor_normal,axis,projection_constants,crt,fp);
            angle_facts=signed_angle;
            if(!signed_angle){auto out=fail(PlayerAccelerationFailure::projection_unavailable,0x44b518,0x438410);out.projection=signed_angle;out.nested_source_boundary=signed_angle.source_boundary;return out;}
            angle=static_cast<std::int32_t>((static_cast<std::uint32_t>(task.orientation.angles[1])+static_cast<std::uint32_t>(*signed_angle.angle))&0xffffU);
            world.target_yaw=angle;
            if(!callees.yaw_blended)return fail(PlayerAccelerationFailure::missing_callee,0x44b52f,0x443e60);
            const auto result=callees.yaw_blended(task,world,player,p,angle,fp);
            if(!result)return failed_callee(result,0x44b52f,0x443e60,temporary,projection_facts,angle_facts);
            rotation_facts=result.rotation;
        }
        world.source_field_34=magnitude;drive=magnitude;
    }
    finish_player_ground_friction(g,drive,extra_y,active,world,player,p,constants,LongitudinalFriction::apply,fp);
    auto result=PlayerAccelerationResult{PlayerAccelerationFailure::none,0,0,g};result.floor_projection=projection_facts;result.projection=angle_facts;result.rotation=rotation_facts;return result;
}

} // namespace sadx
