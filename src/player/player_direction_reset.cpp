#include "player/player_direction_reset.hpp"
#include "player/player_acceleration.hpp"
#include "player/player_acceleration_ground.hpp"
#include "player/player_heading.hpp"
#include "player/player_rotation.hpp"
#include <cstring>
namespace sadx {
GroundAccelerationMutation bind_ground_direction_reset(const PlayerDirectionResetConstants& constants,
    const PlayerDirectionResetRuntime& runtime,PlayerCoordinates& coordinates){
    return [&constants,&runtime,&coordinates](PlayerTaskState& task,PlayerMotionState& world,
        PlayerPhysicsState& player,const PlayerParameters& parameters,X87Context& fp){
        const auto reset=reset_player_direction(task,world,player,parameters,constants,runtime,coordinates,fp);
        PlayerAccelerationResult result;result.direction_reset=reset;
        if(!reset){result.failure=PlayerAccelerationFailure::callee_unavailable;result.callee_address=0x43c130;
            result.source_boundary=reset.source_boundary;result.nested_source_boundary=reset.nested_source_boundary;
            result.input_failure=reset.input_failure;}
        return result;
    };
}
std::optional<PlayerDirectionResetConstants> decode_player_direction_reset_constants(const std::uint8_t* b,std::size_t size){
    if(!b||size!=120)return {};
    const auto path=decode_player_path_constants(b,92);if(!path)return {};
    PlayerDirectionResetConstants out;out.path=path->query;out.floor=path->floor;
    std::memcpy(&out.angle.negative_one,b+92,4);std::memcpy(&out.angle.first_factor,b+96,8);
    std::memcpy(&out.angle.second_factor,b+104,8);std::memcpy(&out.plane_distance_scale,b+112,8);return out;
}
X87Value project_point_onto_plane(const NinjaVector& input,const NinjaVector& point,
    const NinjaVector& normal,NinjaVector* output,double scale,X87Context& fp){
    const auto p_y=fp.multiply_float_memory(fp.from_float(point.y),normal.y);
    const auto p_z=fp.multiply_float_memory(fp.from_float(point.z),normal.z);
    const auto yz=fp.add(p_y,p_z);
    const auto p_x=fp.multiply_float_memory(fp.from_float(normal.x),point.x);
    const auto negative_plane=fp.add(yz,p_x).negated();
    const auto i_y=fp.multiply_float_memory(fp.from_float(normal.y),input.y);
    const auto plus_y=fp.add(negative_plane,i_y);
    const auto i_x=fp.multiply_float_memory(fp.from_float(normal.x),input.x);
    const auto plus_x=fp.add(plus_y,i_x);
    const auto i_z=fp.multiply_float_memory(fp.from_float(input.z),normal.z);
    const float distance=fp.to_float(fp.add(plus_x,i_z));
    const float squared=fp.to_float(scalar_squared_extended(normal,fp));
    if(output){
        const auto factor=fp.divide_float_memory(fp.from_float(distance),squared).negated();
        output->x=fp.to_float(fp.add_float_memory(fp.multiply_float_memory(factor,normal.x),input.x));
        output->y=fp.to_float(fp.add_float_memory(fp.multiply_float_memory(factor,normal.y),input.y));
        output->z=fp.to_float(fp.add_float_memory(fp.multiply_float_memory(factor,normal.z),input.z));
    }
    const auto inverse=fp.reverse_divide_double_memory(signed_sqrt_extended(squared,fp),scale);
    auto absolute=fp.from_float(distance);if(absolute.negative())absolute=absolute.negated();
    return fp.multiply(inverse,absolute);
}
PlayerDirectionResetResult reset_player_direction(PlayerTaskState& task,PlayerMotionState& world,
    PlayerPhysicsState& player,const PlayerParameters& parameters,const PlayerDirectionResetConstants& c,
    const PlayerDirectionResetRuntime& runtime,PlayerCoordinates& coordinates,X87Context& fp){
    PlayerDirectionResetResult result;
    const auto fail=[&](PlayerDirectionResetFailure reason,std::uint32_t address){result.failure=reason;result.source_boundary=address;return result;};
    const auto clear_flag=[&]{task.orientation.flags&=0xdfffU;return result;};
    if(fp.compare_float_memory(fp.from_float(player.motion.speed.x),parameters.dash_speed)==X87Comparison::less){
        const auto gate=get_player_input(input_task_view(task),runtime.input,fp,nullptr,nullptr,runtime.forced_angle);
        if(!gate||!gate.active){result.input_failure=gate.failure;result.nested_source_boundary=gate.source_boundary;return fail(PlayerDirectionResetFailure::input_unavailable,0x43c14e);}
        if(*gate.active)player.motion.speed.x=parameters.dash_speed;
    }
    const auto loaded_speed=fp.from_float(player.motion.speed.x);
    if(!player.path_distance)return fail(PlayerDirectionResetFailure::unavailable_distance,0x43c16a);
    const auto distance=fp.add_float_memory(loaded_speed,*player.path_distance);
    player.path_distance=fp.to_float(distance);
    if(!task.path_resource)return fail(PlayerDirectionResetFailure::unavailable_path_resource,0x43c171);
    result.path_status=PlayerPathStatus{fp.to_float(distance)};
    if(!*task.path_resource)return fail(PlayerDirectionResetFailure::null_path_resource,0x49c338);
    const auto math=coordinates.source_math(fp);
    result.query=query_player_path(**task.path_resource,*result.path_status,c.path,c.floor,runtime.floor,math,fp);
    if(!*result.query){result.nested_source_boundary=result.query->source_boundary;return fail(PlayerDirectionResetFailure::path_query_failed,0x43c179);}
    if(!*result.query->source_return)return clear_flag();
    if(!player.contact_work)return fail(PlayerDirectionResetFailure::unavailable_contact_work,0x43c189);
    const auto& position=result.path_status->position_at20;
    if(!position[0]||!position[1]||!position[2])return fail(PlayerDirectionResetFailure::unavailable_path_position,0x43c18c);
    const auto px=fp.from_float(*position[0]);
    if(!player.contact_work->direction_at_1c)return fail(PlayerDirectionResetFailure::unavailable_contact_direction,0x43c190);
    const auto normal=*player.contact_work->direction_at_1c;
    if(!task.position)return fail(PlayerDirectionResetFailure::unavailable_task_position,0x43c193);
    NinjaVector displacement;
    displacement.x=fp.to_float(fp.subtract_float_memory(px,task.position->x));
    displacement.y=fp.to_float(fp.subtract_float_memory(fp.from_float(*position[1]),task.position->y));
    displacement.z=fp.to_float(fp.subtract_float_memory(fp.from_float(*position[2]),task.position->z));
    NinjaVector projected_delta,projected_speed;
    const NinjaVector zero{0.0F,0.0F,0.0F};
    (void)project_point_onto_plane(displacement,zero,normal,&projected_delta,c.plane_distance_scale,fp);
    result.projected_displacement=projected_delta;
    (void)project_point_onto_plane(world.motion.speed,zero,normal,&projected_speed,c.plane_distance_scale,fp);
    result.projected_velocity=projected_speed;
    if(fp.compare_float_memory(rotation_vector_length(projected_speed,fp),c.path.scalar.zero)==X87Comparison::equal)return clear_flag();
    if(fp.compare_float_memory(rotation_vector_length(projected_delta,fp),c.path.scalar.zero)==X87Comparison::equal)return clear_flag();
    (void)normalize_vector_extended(projected_delta,c.path.scalar,fp);result.projected_displacement=projected_delta;
    (void)normalize_vector_extended(projected_speed,c.path.scalar,fp);result.projected_velocity=projected_speed;
    result.angle=projection_vector_angle(projected_delta,projected_speed,&normal,c.path.scalar,c.angle,runtime.inverse,fp);
    if(!*result.angle){result.nested_source_boundary=result.angle->source_boundary;return fail(PlayerDirectionResetFailure::vector_angle_failed,0x43c272);}
    auto selected=(static_cast<std::uint32_t>(task.orientation.angles[1])+static_cast<std::uint32_t>(*result.angle->angle))&0xffffU;
    if(angle_distance(static_cast<std::int32_t>(selected),world.target_yaw)>0x4000U)selected=(0xffff8000U-selected)&0xffffU;
    world.target_yaw=static_cast<std::int32_t>(selected);task.orientation.angles[1]=static_cast<std::int32_t>(selected);return result;
}
} // namespace sadx
