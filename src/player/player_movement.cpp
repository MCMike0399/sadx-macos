#include "player/player_movement.hpp"

namespace sadx {

PlayerMovementResult run_player_movement(
    const PlayerTaskState& task,const PlayerInputRuntime& input_runtime,
    MotionWork& world,PlayerPhysicsState& player,const PlayerParameters& parameters,
    PlayerCoordinates& coordinates,const FloorAngleConstants& constants,
    const CrtAsinRuntime& math_runtime,X87Context& fp) {
    PlayerMovementResult result{};
    const auto input_task=input_task_view(task);
    PlayerSpeedCallees callees;
    callees.input_gate=[&](const PlayerOrientation&,X87Context& context) {
        ++result.input_query_count;
        const auto queried=get_player_input(input_task,input_runtime,context,nullptr,nullptr,{});
        result.input=queried;
        if (!queried || !queried.active)
            return PlayerInputGateResult{PlayerSpeedFailure::input_gate_unavailable,{}};
        return PlayerInputGateResult{PlayerSpeedFailure::none,*queried.active};
    };
    callees.task_to_world=[&](const PlayerOrientation& orientation,NinjaVector& vector,X87Context& context) {
        const auto transformed=coordinates.task_to_world(&orientation,vector,context);
        result.coordinates=transformed;
        return transformed ? PlayerSpeedFailure::none : PlayerSpeedFailure::task_to_world_transform_unavailable;
    };
    callees.floor_to_world=[&](const NinjaVector& normal,NinjaVector& vector,std::int32_t yaw,X87Context& context) {
        const NormalAnglesProvider provider=[&](const NinjaVector& source,X87Context& arithmetic) {
            const auto angles=floor_normal_angles(source,constants,math_runtime,arithmetic);
            result.floor_angles=angles;
            if (angles.failure!=CrtAsinFailure::none || !angles.angles)
                return NormalAnglesResult{CoordinateFailure::normal_angles_unavailable,{}};
            return NormalAnglesResult{CoordinateFailure::none,*angles.angles};
        };
        const auto transformed=coordinates.floor_to_world(normal,vector,yaw,provider,context);
        result.coordinates=transformed;
        return transformed ? PlayerSpeedFailure::none : PlayerSpeedFailure::floor_to_world_transform_unavailable;
    };
    result.speed=get_player_speed(task.orientation,world,player,parameters,callees,fp);
    return result;
}

} // namespace sadx
