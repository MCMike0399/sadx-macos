#include "player/player_scene_position.hpp"
#include "player/player_heading.hpp"
#include "cpu/crt_integer.hpp"
#include "cpu/x87_transcendental.hpp"
#include <cstring>
#include <stdexcept>

namespace sadx {
float PlayerScenePositionConstants::single(std::uint32_t address)const{
    for(std::size_t i=0;i<float_addresses.size();++i)if(float_addresses[i]==address){float value;std::memcpy(&value,&float_bits[i],4);return value;}
    throw std::out_of_range("Unknown scene-position binary32 source constant");
}
double PlayerScenePositionConstants::wide(std::uint32_t address)const{
    for(std::size_t i=0;i<double_addresses.size();++i)if(double_addresses[i]==address){double value;std::memcpy(&value,&double_bits[i],8);return value;}
    throw std::out_of_range("Unknown scene-position binary64 source constant");
}
std::optional<PlayerScenePositionConstants> decode_scene_position_constants(const std::uint8_t* bytes,std::size_t size){
    if(!bytes||size!=124)return {};PlayerScenePositionConstants value;std::memcpy(value.float_bits.data(),bytes,100);std::memcpy(value.double_bits.data(),bytes+100,24);return value;
}
namespace {
struct Missing{ScenePositionFailure why;std::uint32_t site;};
template<class T>const T& need(const std::optional<T>& value,std::uint32_t site){if(!value)throw Missing{ScenePositionFailure::unavailable_field,site};return *value;}
PlayerScenePositionResult stopped(Missing m){PlayerScenePositionResult r;r.failure=m.why;r.source_boundary=m.site;return r;}
PlayerScenePositionResult complete(){PlayerScenePositionResult r;r.completed=true;return r;}
bool carry(X87Comparison c){return c==X87Comparison::less||c==X87Comparison::unordered;}
}
PlayerScenePositionResult nudge_scene_normal(PlayerTaskState& task,PlayerMotionState* world,const PlayerContactWork& contact,
    const PlayerScenePositionConstants& c,X87Context& fp){
    try{
        const auto& normal=need(contact.vector_at_3c,0x43cb40);auto& position=const_cast<NinjaVector&>(need(task.position,0x43cb49));
        position.x=fp.to_float(fp.add_float_memory(fp.multiply_float_memory(fp.from_float(normal.x),c.single(0x7dccd8)),position.x));
        position.y=fp.to_float(fp.add_float_memory(fp.multiply_float_memory(fp.from_float(normal.y),c.single(0x7dccd8)),position.y));
        position.z=fp.to_float(fp.add_float_memory(fp.multiply_float_memory(fp.from_float(normal.z),c.single(0x7dccd8)),position.z));
        if(!world)throw Missing{ScenePositionFailure::null_motion,0x43cb72};
        const auto x=fp.from_float(normal.x);world->motion.speed.x=fp.to_float(fp.add_float_memory(fp.add(x,x),world->motion.speed.x));
        const auto z=fp.from_float(normal.z);world->motion.speed.z=fp.to_float(fp.add_float_memory(fp.add(z,z),world->motion.speed.z));return complete();
    }catch(Missing m){return stopped(m);}
}
PlayerScenePositionResult move_scene_platform(PlayerTaskState& task,PlayerPhysicsState& player,PlayerContactWork& contact,
    const NinjaVector& offset,const ScenePlatformRuntime& runtime,const CoordinateAxisConstants& axis,X87Context& fp){
    PlayerScenePositionResult r;
    try{
        const auto moving=need(player.moving_task_at_6c,0x43ca4b);if(!moving)throw Missing{ScenePositionFailure::null_moving_task,0x43ca4f};
        if(!moving->force_work)return complete(); // source observed-null task+28
        if(!runtime.platform_work)throw Missing{ScenePositionFailure::unavailable_platform_work,0x43ca4f};
        const auto found=runtime.platform_work->find(moving.get());if(found==runtime.platform_work->end()||!found->second)throw Missing{ScenePositionFailure::unavailable_platform_work,0x43ca4f};
        auto& row=const_cast<ScenePlatformRow&>(need(found->second->rows[task.player_index?1:0],0x43ca93));
        if(fp.compare_float_memory(rotation_vector_length(player.motion.speed,fp),axis.zero)==X87Comparison::equal){
            const auto& position=need(task.position,0x43ca79);
            (void)need(contact.center_at_4,0x43ca7e); // The scene caller has already produced this vector.
            contact.center_at_4->x=fp.to_float(fp.add_float_memory(fp.from_float(position.x),offset.x));
            contact.center_at_4->y=fp.to_float(fp.add_float_memory(fp.from_float(position.y),offset.y));
            contact.center_at_4->z=fp.to_float(fp.add_float_memory(fp.from_float(position.z),offset.z));
        }
        const auto callback=need(row.callback,0x43ca93);
        if(callback){
            const auto saved=need(task.position,0x43ca9d);r.temporary_vector=saved;task.position=need(contact.center_at_4,0x43cab4);
            if(!runtime.players||task.player_index>=runtime.players->tasks.size())throw Missing{ScenePositionFailure::unavailable_registry_owner,0x43caca};
            const auto player_task=need(runtime.players->tasks[task.player_index],0x43caca);
            const auto result=callback(player_task,task,row,fp);
            if(!result){r.platform_callback=std::make_shared<PlayerScenePositionResult>(result);r.failure=ScenePositionFailure::platform_callback_failed;r.source_boundary=0x43cad4;r.nested_source_boundary=result.source_boundary;return r;}
            task.position=saved;
        }
        auto& center=const_cast<NinjaVector&>(need(contact.center_at_4,0x43caf0));
        center.x=fp.to_float(fp.add_float_memory(fp.from_float(need(row.displacement,0x43caed).x),center.x));
        center.y=fp.to_float(fp.add_float_memory(fp.from_float(need(row.displacement,0x43caf6).y),center.y));
        center.z=fp.to_float(fp.add_float_memory(fp.from_float(need(row.displacement,0x43caff).z),center.z));
        for(std::size_t i=0;i<3;++i)task.orientation.angles[i]=static_cast<std::int32_t>((static_cast<std::uint32_t>(need(row.angle_delta,0x43cb08)[i])+static_cast<std::uint32_t>(task.orientation.angles[i]))&0xffffU);
        r.completed=true;return r;
    }catch(Missing m){r.failure=m.why;r.source_boundary=m.site;return r;}
}
PlayerScenePositionResult rotate_scene_floor(PlayerTaskState& task,PlayerPhysicsState& player,const PlayerParameters& p,
    const PlayerContactWork& contact,const PlayerScenePositionConstants& c,const std::optional<NinjaVector>& world_gravity,
    PlayerCoordinates& coordinates,X87Context& fp){
    PlayerScenePositionResult r;
    try{
        const auto& gravity=need(world_gravity,0x444f24);const auto& direction=need(contact.direction_at_1c,0x444f24);const auto& normal=need(contact.vector_at_48,0x444f34);
        const float old_dot=fp.to_float(inner_product_extended(gravity,direction,fp));
        const auto comparison=fp.compare_float_memory(inner_product_extended(gravity,normal,fp),old_dot);
        // TEST AH,05/JNP takes only C0 without C2, the ordered-less case.
        if(comparison==X87Comparison::less)goto copy_angles;
        if(!(need(player.flags_at_10,0x444f4a)&1U)||(*player.flags_at_10&0x2000U))goto copy_angles;
        {
            auto local=normal;r.coordinate=coordinates.world_to_player(task.orientation,local,fp);if(!r.coordinate){r.failure=ScenePositionFailure::coordinate_failed;r.source_boundary=0x444f7e;r.nested_source_boundary=r.coordinate.source_boundary;return r;}
            const auto x=fp.multiply_float_memory(fp.from_float(local.x),local.x);const auto z=fp.multiply_float_memory(fp.from_float(local.z),local.z);
            local.x=fp.to_float(signed_sqrt_extended(fp.to_float(fp.add(x,z)),fp));local.z=0;
            const auto loaded_x=fp.from_float(local.x),loaded_y=fp.from_float(local.y);
            auto angle=X87Transcendental::fpatan_bochs_p5_p6(fp,loaded_x,loaded_y);
            angle=fp.multiply_double_memory(fp.multiply_double_memory(angle,c.wide(0x7dccd0)),c.wide(0x7dccc8));
            auto step=crt_ftol_low32(angle,fp)&0xffffU;if(step>0x8000U)step=0x10000U-step;
            const auto cf=[&](float a,float b){return fp.compare_float_memory(fp.from_float(a),b);};
            if(!(task.orientation.flags&0x2000U)&&carry(cf(player.motion.speed.x,p.dash_speed))){
                if(!carry(cf(player.motion.speed.x,p.rush_speed))){if(step>0x2000U)step=0x2000U;}
                else if(!carry(cf(player.motion.speed.x,p.run_speed))){if(step>0x1500U)step=0x1500U;}
                else if(!carry(cf(player.motion.speed.x,p.jog_speed))){if(step>0x1000U)step=0x1000U;}
                else step=0x400;
            }
            task.orientation.angles[0]=adjust_angle(task.orientation.angles[0],need(contact.angle_x_at_34,0x445063),static_cast<std::int32_t>(step));
            task.orientation.angles[2]=adjust_angle(task.orientation.angles[2],need(contact.angle_z_at_38,0x445074),static_cast<std::int32_t>(step));r.completed=true;return r;
        }
copy_angles:
        task.orientation.angles[0]=need(contact.angle_x_at_34,0x44508e);task.orientation.angles[2]=need(contact.angle_z_at_38,0x445094);r.completed=true;return r;
    }catch(Missing m){r.failure=m.why;r.source_boundary=m.site;return r;}
    catch(const CoordinateError& error){r.failure=ScenePositionFailure::coordinate_failed;r.coordinate.failure=error.failure();r.source_boundary=0x444f7e;return r;}
}
} // namespace sadx
