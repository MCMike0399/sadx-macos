#include "player/player_acceleration_air.hpp"
#include "player/player_heading.hpp"

namespace sadx {
namespace {
bool odd_status_parity(X87Comparison comparison,unsigned mask) {
    auto bits=static_cast<unsigned>(x87_comparison_status(comparison)>>8U)&mask;
    unsigned count=0;while(bits){count+=bits&1U;bits>>=1U;}
    return (count&1U)!=0;
}
}

AirAccelerationResult get_player_acceleration_air(
    PlayerTaskState& task,PlayerMotionState& world,PlayerPhysicsState& player,
    const PlayerParameters& p,const AirAccelerationConstants& constants,
    const PlayerAccelerationRuntime& runtime,const AirAccelerationCallees& callees,X87Context& fp) {
    if((task.orientation.flags&0x4000U)!=0) {
        if(!callees.event_acceleration)
            return {{PlayerAccelerationFailure::missing_callee,0x44b9e3,0x447b90},{}};
        const auto result=callees.event_acceleration(task,world,player,p,fp);
        return {result,{}}; // Preserve complete source callee failure facts.
    }
    if(!runtime.world_gravity)
        return {{PlayerAccelerationFailure::unavailable_gravity,0x44b9f0},{}};
    const auto product=[&](float first,float memory) {
        return fp.multiply_float_memory(fp.from_float(first),memory);
    };
    NinjaVector acceleration;
    acceleration.x=fp.to_float(fp.add_float_memory(product(runtime.world_gravity->x,p.weight),world.motion.acceleration.x));
    acceleration.y=fp.to_float(fp.add_float_memory(product(runtime.world_gravity->y,p.weight),world.motion.acceleration.y));
    acceleration.z=fp.to_float(fp.add_float_memory(product(runtime.world_gravity->z,p.weight),world.motion.acceleration.z));
    const auto failure=[&](PlayerAccelerationFailure kind,std::uint32_t site,std::uint32_t callee=0) {
        return AirAccelerationResult{{kind,site,callee,acceleration},{}};
    };
    if(!callees.world_to_player)
        return failure(PlayerAccelerationFailure::missing_world_to_player,0x44ba53,0x43ec00);
    const auto transformed=callees.world_to_player(task.orientation,acceleration,fp);
    if(!transformed) {
        auto result=failure(PlayerAccelerationFailure::coordinate_unavailable,0x44ba53,0x43ec00);
        result.acceleration.coordinate_failure=transformed.failure;
        result.acceleration.nested_source_boundary=transformed.source_boundary;
        return result;
    }
    acceleration.x=fp.to_float(fp.add_float_memory(product(p.air_resist_air,player.motion.speed.x),acceleration.x));
    acceleration.y=fp.to_float(fp.add_float_memory(product(p.air_resist_y,player.motion.speed.y),acceleration.y));
    acceleration.z=fp.to_float(fp.add_float_memory(product(p.air_resist_z,player.motion.speed.z),acceleration.z));
    std::int32_t requested_angle;float magnitude;
    const auto input=get_player_input(input_task_view(task),runtime.input,fp,&requested_angle,&magnitude,runtime.forced_angle);
    if(!input || !input.active) {
        auto result=failure(PlayerAccelerationFailure::input_unavailable,0x44ba96,0x43bdc0);
        result.acceleration.input_failure=input.failure;
        result.acceleration.nested_source_boundary=input.source_boundary;
        return result;
    }
    // Inactive input does not consume output scratch; active input requires
    // both actual source stores, rather than substituting zeros for old bytes.
    if(*input.active && input.outputs_written!=3)
        return failure(PlayerAccelerationFailure::input_outputs_unavailable,0x44bab4,0x43bdc0);
    std::optional<X87Value> drive;
    std::optional<PlayerRotationResult> rotation;
    if(!*input.active) {
        world.target_yaw=task.orientation.angles[1];
        drive=fp.from_float(constants.zero);
    } else {
        const auto distance=angle_distance(task.orientation.angles[1],requested_angle);
        const auto speed_test=fp.compare_float_memory(fp.from_float(player.motion.speed.x),p.run_speed);
        if(speed_test==X87Comparison::greater && distance>0x6000U) {
            // Retain the previous target yaw on this braking branch.
            magnitude=fp.to_float(product(magnitude,p.air_break));
        } else {
            world.target_yaw=requested_angle;
            if(distance>0x1000U) magnitude=0.0F; // actual MOV literal, not source-memory zero
            else {
                const auto vertical=fp.compare_float_memory(fp.from_float(world.motion.speed.y),constants.zero);
                auto control=product(magnitude,p.air_accel);
                if(odd_status_parity(vertical,0x05)) control=fp.add(control,control);
                magnitude=fp.to_float(control);
            }
        }
        if(!callees.yaw_regular)
            return failure(PlayerAccelerationFailure::missing_callee,0x44bb22,0x443c50);
        const auto turned=callees.yaw_regular(task,world,player,world.target_yaw,fp);
        rotation=turned;
        if(!turned) {
            auto result=failure(PlayerAccelerationFailure::callee_unavailable,0x44bb22,0x443c50);
            result.rotation=turned;
            result.acceleration.nested_source_boundary=turned.source_boundary;
            result.acceleration.coordinate_failure=turned.coordinate_failure;
            result.acceleration.input_failure=turned.input_failure;
            return result;
        }
        drive=fp.from_float(magnitude); // source reload after yaw callee
    }
    acceleration.x=fp.to_float(fp.add_float_memory(*drive,acceleration.x));
    player.motion.acceleration=acceleration; // source X/Y/Z MOV stores; no extra Y/Z FPU stores
    return {{},rotation};
}

} // namespace sadx
