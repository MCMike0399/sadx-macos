#include "player/player_reaction.hpp"
#include "player/player_heading.hpp"
#include <cstring>

namespace sadx {
namespace {
std::int32_t signed_word(std::uint32_t word){std::int32_t result;std::memcpy(&result,&word,4);return result;}
}
std::optional<PlayerReactionConstants> decode_player_reaction_constants(const std::uint8_t* bytes,std::size_t size){
    if(!bytes||size!=40)return std::nullopt;
    PlayerReactionConstants result;
    std::memcpy(&result.plane.axis.zero,bytes,4);std::memcpy(&result.plane.axis.one,bytes+4,4);
    std::memcpy(&result.plane.parallel_cosine,bytes+8,4);std::memcpy(&result.plane.distance_scale,bytes+12,8);
    std::memcpy(&result.angle.negative_one,bytes+20,4);std::memcpy(&result.angle.first_factor,bytes+24,8);
    std::memcpy(&result.angle.second_factor,bytes+32,8);return result;
}
PlayerReactionResult react_player_to_contact(PlayerTaskState& task,PlayerMotionState* world,
    PlayerPhysicsState& player,PlayerPositionCollisionScene& scene,const PlayerReactionConstants& c,
    const CrtAsinRuntime& runtime,const decltype(PlayerPositionCallees::normal)& normal,X87Context& fp){
    PlayerReactionResult result;
    auto fail=[&](PlayerReactionFailure failure,std::uint32_t boundary){
        result.failure=failure;result.source_boundary=boundary;return result;
    };
    if((task.orientation.flags&3)==0)return result;
    if(!world)return fail(PlayerReactionFailure::null_motion,0x43c59a);
    const auto speed=fp.compare_float_memory(rotation_vector_length(world->motion.speed,fp),c.plane.axis.one);
    if(speed==X87Comparison::less)return result; // TEST AH,5 / JNP; equality continues
    if(!scene.task_collision)return fail(PlayerReactionFailure::unknown_collision_reference,0x43c5b0);
    if(!scene.task_collision->id)return fail(PlayerReactionFailure::null_collision_reference,0x43c5b3);
    const auto work=scene.works.find(*scene.task_collision->id);
    if(work==scene.works.end())return fail(PlayerReactionFailure::unresolved_collision_work,0x43c5b3);
    if(!work->second.flags)return fail(PlayerReactionFailure::unknown_collision_flags,0x43c5b3);
    if((*work->second.flags&0x10)==0 || (task.orientation.flags&4)!=0 || (*work->second.flags&4)==0)return result;
    if(!normal)return fail(PlayerReactionFailure::missing_normal_owner,0x43c5d8);
    const auto queried=normal(task,scene,fp);
    if(queried.failure!=PlayerPositionFailure::none){
        result.normal_failure=queried.failure;result.nested_source_boundary=queried.source_boundary;
        return fail(PlayerReactionFailure::normal_owner_failed,0x43c5d8);
    }
    if(!queried.normal)return fail(PlayerReactionFailure::unknown_normal,0x43c5dd);
    auto local_normal=*queried.normal;
    player.source_vector_at_44=local_normal; // source MOV stores precede normalization
    if(fp.compare_float_memory(rotation_vector_length(local_normal,fp),c.plane.axis.zero)==X87Comparison::equal)
        local_normal={0,0,0};
    else (void)normalize_vector_extended(local_normal,c.plane.axis,fp);
    // The source intentionally constructs its plane from the raw +44 copy,
    // not the normalized local. Preserve even this otherwise unused arithmetic.
    if(!player.floor_normal)return fail(PlayerReactionFailure::unknown_floor_normal,0x43c645);
    const NinjaPlane first{{0,0,0},*player.source_vector_at_44},second{{0,0,0},*player.floor_normal};
    NinjaPlane intersection{};
    (void)intersect_source_planes(first,second,&intersection,c.plane,fp);
    result.intersection=intersection;
    auto old_direction=world->motion.speed;
    if(fp.compare_float_memory(rotation_vector_length(old_direction,fp),c.plane.axis.zero)!=X87Comparison::equal)
        (void)normalize_vector_extended(old_direction,c.plane.axis,fp);
    const auto direction=fp.compare_float_memory(inner_product_extended(intersection.normal,old_direction,fp),c.plane.axis.zero);
    if(direction==X87Comparison::less){
        intersection.normal.x=fp.to_float(fp.from_float(intersection.normal.x).negated());
        intersection.normal.y=fp.to_float(fp.from_float(intersection.normal.y).negated());
        intersection.normal.z=fp.to_float(fp.from_float(intersection.normal.z).negated());
    }
    const auto length=rotation_vector_length(world->motion.speed,fp);
    const auto prior_speed=world->motion.speed; // all three source MOVs before first store
    world->motion.speed.x=fp.to_float(fp.multiply(fp.from_float(intersection.normal.x),length));
    world->motion.speed.y=fp.to_float(fp.multiply(fp.from_float(intersection.normal.y),length));
    world->motion.speed.z=fp.to_float(fp.multiply_float_memory(length,intersection.normal.z));
    auto before=prior_speed,after=world->motion.speed;
    (void)normalize_vector_extended(before,c.plane.axis,fp);
    (void)normalize_vector_extended(after,c.plane.axis,fp);
    result.angle=projection_vector_angle(before,after,&*player.floor_normal,c.plane.axis,c.angle,runtime,fp);
    if(!*result.angle){
        result.nested_source_boundary=result.angle->source_boundary;
        return fail(PlayerReactionFailure::angle_failed,0x43c78c);
    }
    auto yaw=static_cast<std::uint32_t>(task.orientation.angles[1])-static_cast<std::uint32_t>(*result.angle->angle);
    task.orientation.angles[1]=signed_word(yaw);
    if(angle_distance(world->target_yaw,task.orientation.angles[1])>0x6000U){
        yaw+=0x8000U;task.orientation.angles[1]=signed_word(yaw);
    }
    world->target_yaw=task.orientation.angles[1];return result;
}
decltype(PlayerPositionCallees::reaction) bind_player_contact_reaction(
    PlayerReactionConstants constants,const CrtAsinRuntime& runtime,decltype(PlayerPositionCallees::normal) normal){
    return [constants,&runtime,normal=std::move(normal)](
            PlayerTaskState& task,PlayerMotionState* world,PlayerPhysicsState& player,
            PlayerPositionCollisionScene& scene,X87Context& fp){
        const auto result=std::make_shared<PlayerReactionResult>(
            react_player_to_contact(task,world,player,scene,constants,runtime,normal,fp));
        return PositionEffectResult{*result?PlayerPositionFailure::none:PlayerPositionFailure::reaction_owner_failed,
                                    result->source_boundary,result};
    };
}
} // namespace sadx
