#include "player/player_rotation.hpp"
#include "cpu/crt_integer.hpp"
#include "cpu/x87_transcendental.hpp"
#include <cstring>
#include <stdexcept>

namespace sadx {
float PlayerRotationConstants::single(std::uint32_t address) const {
    for(std::size_t i=0;i<float_addresses.size();++i) if(float_addresses[i]==address) {
        float value;std::memcpy(&value,&float_bits[i],4);return value;
    }
    throw std::out_of_range("unmapped source rotation binary32 constant");
}
double PlayerRotationConstants::wide(std::uint32_t address) const {
    for(std::size_t i=0;i<double_addresses.size();++i) if(double_addresses[i]==address) {
        double value;std::memcpy(&value,&double_bits[i],8);return value;
    }
    throw std::out_of_range("unmapped source rotation binary64 constant");
}
namespace {
std::int32_t signed_bits(std::uint32_t value) { std::int32_t out;std::memcpy(&out,&value,4);return out; }
PlayerRotationResult coordinate_failure(CoordinateResult result,std::uint32_t boundary,const NinjaVector& v) {
    return {PlayerRotationFailure::coordinate_unavailable,boundary,result.failure,PlayerInputFailure::none,v,result.source_boundary};
}
X87Comparison compare(float a,float b,X87Context& fp) { return fp.compare_float_memory(fp.from_float(a),b); }
PlayerInputResult input(const PlayerTaskState& task,const PlayerRotationRuntime& r,X87Context& fp) {
    return get_player_input(input_task_view(task),r.input,fp,nullptr,nullptr,r.forced_angle);
}
PlayerRotationResult start_turn(PlayerTaskState& task,PlayerPhysicsState& player,
                               NinjaVector& v,PlayerCoordinates& coordinates,X87Context& fp,
                               std::uint32_t boundary) {
    v=player.motion.speed;
    const auto result=coordinates.task_to_world(&task.orientation,v,fp);
    return result?PlayerRotationResult{}:coordinate_failure(result,boundary,v);
}
PlayerRotationResult finish_turn(PlayerTaskState& task,NinjaVector& v,PlayerCoordinates& coordinates,
                                X87Context& fp,std::uint32_t boundary) {
    const auto result=coordinates.world_to_player(task.orientation,v,fp);
    return result?PlayerRotationResult{}:coordinate_failure(result,boundary,v);
}
void mix_regular_weights(PlayerPhysicsState& player,const NinjaVector& v,
                         X87Value old_weight,X87Value new_weight,X87Context& fp) {
    const auto x=fp.multiply(fp.from_float(v.x),new_weight);
    const auto old_x=fp.multiply_float_memory(old_weight,player.motion.speed.x);
    player.motion.speed.x=fp.to_float(fp.add(x,old_x));
    const auto old_y=fp.multiply_float_memory(old_weight,player.motion.speed.y);
    const auto y=fp.multiply_float_memory(new_weight,v.y);
    player.motion.speed.y=fp.to_float(fp.add(old_y,y));
    const auto z=fp.multiply_float_memory(new_weight,v.z);
    const auto old_z=fp.multiply_float_memory(old_weight,player.motion.speed.z);
    player.motion.speed.z=fp.to_float(fp.add(z,old_z));
}
std::int32_t atan_units(float y,float x,const PlayerRotationConstants& c,X87Context& fp) {
    const auto loaded_y=fp.from_float(y),loaded_x=fp.from_float(x);
    const auto angle=X87Transcendental::fpatan_bochs_p5_p6(fp,loaded_y,loaded_x);
    const auto scaled=fp.multiply_double_memory(angle,c.wide(0x7dccd0));
    return signed_bits(crt_ftol_low32(fp.multiply_double_memory(scaled,c.wide(0x7dcce8)),fp));
}
float source_absolute(float x,const PlayerRotationConstants& c,X87Context& fp) {
    if(compare(x,c.single(0x7dcb2c),fp)==X87Comparison::less)
        return fp.to_float(fp.from_float(x).negated());
    return x;
}
}

PlayerRotationResult rotate_player_regular(PlayerTaskState& task,PlayerMotionState& world,
    PlayerPhysicsState& player,std::int32_t target,const PlayerRotationConstants& c,
    const PlayerRotationRuntime& runtime,PlayerCoordinates& coordinates,X87Context& fp) {
    NinjaVector v;auto result=start_turn(task,player,v,coordinates,fp,0x443c78);if(!result)return result;
    const auto distance=angle_distance(task.orientation.angles[1],target);
    const auto step=distance>0x2000U?0x800U:distance>0x1000U?distance>>2U:distance>>3U;
    task.orientation.angles[1]=adjust_angle(task.orientation.angles[1],target,static_cast<std::int32_t>(step));
    result=finish_turn(task,v,coordinates,fp,0x443cc9);if(!result)return result;
    if((task.orientation.flags&3U)==0) {
        const auto new_x=fp.multiply_float_memory(fp.from_float(v.x),c.single(0x7dfff0));
        const auto old_x=fp.multiply_float_memory(fp.from_float(player.motion.speed.x),c.single(0x7dd568));
        player.motion.speed.x=fp.to_float(fp.add(new_x,old_x));
        const auto old_y=fp.multiply_float_memory(fp.from_float(player.motion.speed.y),c.single(0x7dd568));
        const auto new_y=fp.multiply_float_memory(fp.from_float(v.y),c.single(0x7dfff0));
        player.motion.speed.y=fp.to_float(fp.add(old_y,new_y));
        const auto old_z=fp.multiply_float_memory(fp.from_float(player.motion.speed.z),c.single(0x7dd568));
        const auto new_z=fp.multiply_float_memory(fp.from_float(v.z),c.single(0x7dfff0));
        player.motion.speed.z=fp.to_float(fp.add(old_z,new_z));return {};
    }
    const auto gate=input(task,runtime,fp);
    if(!gate || !gate.active) return {PlayerRotationFailure::input_unavailable,0x443d29,
        CoordinateFailure::none,gate.failure,v,gate.source_boundary};
    const auto address=!*gate.active?0x7e0070U:
        compare(player.dotp,c.single(0x7dd550),fp)==X87Comparison::greater?0x7e0118U:0x7dcc98U;
    auto old_weight=fp.from_float(c.single(address));
    if(compare(world.friction_scale,c.single(0x7dcb10),fp)==X87Comparison::less)
        old_weight=fp.multiply_float_memory(fp.from_float(world.friction_scale),c.single(address));
    const auto new_weight=fp.subtract(fp.from_float(c.single(0x7dcb10)),old_weight);
    mix_regular_weights(player,v,old_weight,new_weight,fp);return {};
}

PlayerRotationResult rotate_player_blended(PlayerTaskState& task,PlayerPhysicsState& player,
    const PlayerParameters& parameters,std::int32_t target,const PlayerRotationConstants& c,
    PlayerCoordinates& coordinates,X87Context& fp) {
    NinjaVector v;auto result=start_turn(task,player,v,coordinates,fp,0x443e83);if(!result)return result;
    std::uint32_t step=0x100;
    if(compare(player.motion.speed.x,parameters.dash_speed,fp)==X87Comparison::greater) {
        const auto difference=fp.subtract_float_memory(fp.from_float(player.motion.speed.x),parameters.dash_speed);
        const float squared=fp.to_float(fp.multiply_float_memory(difference,c.single(0x7e0120)));
        const auto root=signed_sqrt_extended(squared,fp);
        step-=crt_ftol_low32(fp.multiply_float_memory(root,c.single(0x7dce68)),fp);
    }
    task.orientation.angles[1]=adjust_angle(task.orientation.angles[1],target,signed_bits(step));
    result=finish_turn(task,v,coordinates,fp,0x443ee0);if(!result)return result;
    const bool steep=compare(player.dotp,c.single(0x7dd550),fp)==X87Comparison::greater;
    const auto old_weight=fp.from_float(c.single(steep?0x7e0118:0x7dcc98));
    const auto new_weight=fp.from_float(c.single(steep?0x7e011c:0x7dcc98));
    const auto new_x=fp.multiply(fp.from_float(v.x),new_weight);
    const auto old_x=fp.multiply_float_memory(old_weight,player.motion.speed.x);
    player.motion.speed.x=fp.to_float(fp.add(new_x,old_x));
    const auto old_y=fp.multiply_float_memory(old_weight,player.motion.speed.y);
    const auto new_y=fp.multiply(fp.from_float(v.y),new_weight);
    player.motion.speed.y=fp.to_float(fp.add(old_y,new_y));
    const auto old_z=fp.multiply_float_memory(old_weight,player.motion.speed.z);
    const auto new_z=fp.multiply(fp.from_float(v.z),new_weight);
    player.motion.speed.z=fp.to_float(fp.add(old_z,new_z));return {};
}

X87Value rotation_vector_length(const NinjaVector& v,X87Context& fp) {
    const auto x=fp.from_float(v.x);const auto xx=fp.multiply(x,x);
    const auto y=fp.from_float(v.y);const auto yy=fp.multiply(y,y);
    const auto z=fp.from_float(v.z);const auto zz=fp.multiply(z,z);
    const auto zy=fp.add(zz,yy);
    const float squared=fp.to_float(fp.add(zy,xx));
    return signed_sqrt_extended(squared,fp);
}

PlayerRotationResult rotate_player_passive(PlayerTaskState& task,PlayerMotionState& world,
    PlayerPhysicsState& player,const PlayerParameters& p,const PlayerRotationConstants& c,
    const PlayerRotationRuntime& r,PlayerCoordinates& coordinates,X87Context& fp) {
    if(!r.passed_player_no_control_timer)return {PlayerRotationFailure::unavailable_timer,0x4491e8};
    if(*r.passed_player_no_control_timer!=0 || (player.attributes&0x4000U)!=0)return {};
    if(!r.world_gravity)return {PlayerRotationFailure::unavailable_gravity,0x4491ff};
    NinjaVector gravity=*r.world_gravity,velocity=world.motion.speed;
    if(fp.compare_float_memory(rotation_vector_length(velocity,fp),p.jog_speed)==X87Comparison::greater) {
        (void)normalize_vector_extended(velocity,{c.single(0x7dcb2c),c.single(0x7dcb10)},fp);
        const auto dot=inner_product_extended(*r.world_gravity,velocity,fp);
        const auto cmp=fp.compare_double_memory(dot,c.wide(0x7e0168));
        if(cmp==X87Comparison::less)return {};
    }
    const auto transformed=coordinates.world_to_player(task.orientation,gravity,fp);
    if(!transformed)return coordinate_failure(transformed,0x449287,gravity);
    if(compare(gravity.y,c.single(0x7dcb2c),fp)==X87Comparison::less &&
       compare(gravity.y,c.single(0x7e0164),fp)==X87Comparison::greater) {
        gravity.x=source_absolute(gravity.x,c,fp);
        const auto offset=atan_units(gravity.z,gravity.x,c,fp);
        const auto target=signed_bits(static_cast<std::uint32_t>(task.orientation.angles[1])-static_cast<std::uint32_t>(offset));
        gravity.z=source_absolute(gravity.z,c,fp);
        const auto factor=c.single((task.orientation.flags&0x100U)!=0?0x7dccf4:0x7e0160);
        const auto step=crt_ftol_low32(fp.multiply_float_memory(fp.from_float(gravity.z),factor),fp);
        world.target_yaw=adjust_angle(world.target_yaw,target,signed_bits(step));
    } else world.target_yaw=task.orientation.angles[1];
    return rotate_player_regular(task,world,player,world.target_yaw,c,r,coordinates,fp);
}

PlayerRotationResult rotate_player_attribute(PlayerTaskState& task,PlayerMotionState& world,
    PlayerPhysicsState& player,const PlayerParameters& p,const PlayerRotationConstants& c,
    const PlayerRotationRuntime& r,PlayerCoordinates& coordinates,X87Context& fp) {
    if(!r.passed_player_no_control_timer)return {PlayerRotationFailure::unavailable_timer,0x449388};
    if(*r.passed_player_no_control_timer!=0)return {};
    if(!r.world_gravity)return {PlayerRotationFailure::unavailable_gravity,0x449396};
    NinjaVector gravity=*r.world_gravity,velocity=world.motion.speed;
    if(fp.compare_float_memory(rotation_vector_length(velocity,fp),p.jog_speed)!=X87Comparison::greater)return {};
    (void)normalize_vector_extended(velocity,{c.single(0x7dcb2c),c.single(0x7dcb10)},fp);
    if(fp.compare_double_memory(inner_product_extended(*r.world_gravity,velocity,fp),c.wide(0x7e0168))==X87Comparison::less)return {};
    const auto transformed=coordinates.world_to_player(task.orientation,gravity,fp);
    if(!transformed)return coordinate_failure(transformed,0x449418,gravity);
    if(compare(gravity.y,c.single(0x7e0164),fp)==X87Comparison::greater) {
        gravity.x=source_absolute(gravity.x,c,fp);
        const auto offset=atan_units(gravity.z,gravity.x,c,fp);
        const auto target=signed_bits(static_cast<std::uint32_t>(task.orientation.angles[1])-static_cast<std::uint32_t>(offset));
        gravity.z=source_absolute(gravity.z,c,fp);
        const bool flagged=(task.orientation.flags&0x100U)!=0;
        auto change=flagged?fp.divide_float_memory(fp.from_float(player.motion.speed.x),p.jog_speed):
            fp.multiply_float_memory(fp.from_float(gravity.z),c.single(0x7dcb60));
        if(flagged) {
            change=fp.multiply_float_memory(change,gravity.z);
            change=fp.multiply_float_memory(change,c.single(0x7e0170));
        }
        world.target_yaw=adjust_angle(world.target_yaw,target,signed_bits(crt_ftol_low32(change,fp)));
    } else world.target_yaw=task.orientation.angles[1];
    const auto turned=turn_player_heading(task.orientation,player.motion,world.target_yaw,coordinates,fp);
    return turned?PlayerRotationResult{}:coordinate_failure(turned.coordinate,0x4494e1,
        turned.temporary_vector.value_or(player.motion.speed));
}
} // namespace sadx
