#include "player/player_scene_position.hpp"
#include "player/player_heading.hpp"
#include "cpu/crt_integer.hpp"
#include <cstring>
#include <unordered_set>

namespace sadx {
namespace {
struct Stopped{};
std::int32_t signed_bits(std::uint32_t value){std::int32_t result;std::memcpy(&result,&value,4);return result;}
bool carry(X87Comparison c){return c==X87Comparison::less||c==X87Comparison::unordered;}
bool parity41(X87Comparison c){return c==X87Comparison::greater||c==X87Comparison::unordered;}
struct SceneOperation {
    PlayerTaskState& task;PlayerMotionState* world;PlayerPhysicsState& player;PlayerParameters& parameters;
    PlayerSceneLandscape& scene;const PlayerScenePositionConstants& constants;PlayerScenePositionRuntime& runtime;
    PlayerCoordinates& coordinates;X87Context& fp;PlayerScenePositionResult result;
    PlayerContactWork* contact=nullptr;
    SceneOperation(PlayerTaskState& t,PlayerMotionState* w,PlayerPhysicsState& p,PlayerParameters& values,
        PlayerSceneLandscape& s,const PlayerScenePositionConstants& c,PlayerScenePositionRuntime& r,
        PlayerCoordinates& coordinate,X87Context& context):task(t),world(w),player(p),parameters(values),scene(s),constants(c),runtime(r),coordinates(coordinate),fp(context){}
    [[noreturn]]void stop(ScenePositionFailure why,std::uint32_t site,std::uint32_t nested=0){result.failure=why;result.source_boundary=site;result.nested_source_boundary=nested;throw Stopped{};}
    template<class T>T& need(std::optional<T>& value,std::uint32_t site){if(!value)stop(ScenePositionFailure::unavailable_field,site);return *value;}
    template<class T>const T& need(const std::optional<T>& value,std::uint32_t site){if(!value)stop(ScenePositionFailure::unavailable_field,site);return *value;}
    PlayerMotionState& motion(std::uint32_t site){if(!world)stop(ScenePositionFailure::null_motion,site);return *world;}
    float c(std::uint32_t address)const{return constants.single(address);}
    X87Value load(float value){return fp.from_float(value);}
    X87Comparison compare(float a,float b){return fp.compare_float_memory(load(a),b);}
    X87Value length(const NinjaVector& value){return rotation_vector_length(value,fp);}
    X87Value dot(const NinjaVector& a,const NinjaVector& b){return inner_product_extended(a,b,fp);}
    void normalize(NinjaVector& v){(void)normalize_vector_extended(v,runtime.axis_constants,fp);}
    void coordinate(CoordinateResult r,std::uint32_t site){result.coordinate=r;if(!r)stop(ScenePositionFailure::coordinate_failed,site,r.source_boundary);}
    void leaf(PlayerScenePositionResult r,std::uint32_t site){if(!r){result.platform_callback=std::make_shared<PlayerScenePositionResult>(r);stop(r.failure,site,r.source_boundary);}}
    std::int32_t angle(const NinjaVector& a,const NinjaVector& b,const NinjaVector* sign,std::uint32_t site){
        result.projection=projection_vector_angle(a,b,sign,runtime.response_constants.plane.axis,runtime.response_constants.angle,runtime.response_math.inverse_math,fp);
        if(!*result.projection)stop(ScenePositionFailure::projection_failed,site,result.projection->source_boundary);return *result.projection->angle;
    }
    bool query(const SceneLandEntry& entry,std::uint32_t site){
        const auto object=need(entry.object,site);
        result.mesh_query=query_mesh_object_contacts(*contact,scene,object,runtime.contacts,runtime.query_constants,
            runtime.floor_constants,runtime.axis_constants,runtime.query_math,coordinates,fp);
        if(!*result.mesh_query)stop(ScenePositionFailure::mesh_query_failed,site,result.mesh_query->source_boundary);return *result.mesh_query->outcome!=0;
    }
    std::int32_t respond(PlayerContactResponseEntry entry,std::uint32_t attributes,std::uint32_t site){
        result.response=respond_player_contacts(entry,*contact,runtime.contacts,{&task,world,&player,&parameters,attributes},runtime.response_constants,runtime.response_math,fp);
        if(!*result.response)stop(ScenePositionFailure::response_failed,site,result.response->source_boundary);return *result.response->outcome;
    }
    void attributes(std::uint32_t incoming,std::uint32_t site){
        result.attributes=apply_player_contact_attributes(task,world,player,parameters,runtime.parameter_defaults,incoming,runtime.attribute_constants,fp);
        if(!*result.attributes)stop(ScenePositionFailure::attributes_failed,site,result.attributes->source_boundary);
    }
    PlayerMotionState& registered_motion(std::uint32_t site){
        if(!runtime.platform.players||task.player_index>=runtime.platform.players->motion_work.size())stop(ScenePositionFailure::unavailable_registry_owner,site);
        const auto& selected=need(runtime.platform.players->motion_work[task.player_index],site);if(!selected)stop(ScenePositionFailure::null_motion,site);return *selected;
    }
    void subtract_angle(std::int32_t angle_value){task.orientation.angles[1]=signed_bits(static_cast<std::uint32_t>(task.orientation.angles[1])-static_cast<std::uint32_t>(angle_value));}
    void align_motion_yaw(std::uint32_t site){auto& registered=registered_motion(site);
        if(angle_distance(registered.target_yaw,task.orientation.angles[1])>0x6000U)task.orientation.angles[1]=signed_bits(static_cast<std::uint32_t>(task.orientation.angles[1])+0x8000U);
        registered.target_yaw=task.orientation.angles[1];
    }
    void scale_retained(NinjaVector& output,const NinjaVector& direction,X87Value scale){
        output.x=fp.to_float(fp.multiply(load(direction.x),scale));output.y=fp.to_float(fp.multiply(load(direction.y),scale));output.z=fp.to_float(fp.multiply_float_memory(scale,direction.z));
    }
};

// Actual43dca0 custom ABI: movement direction is EAX, contact is ECX, task
// and player are stack arguments. No guest register storage is retained here.
void slide_scene_wall(SceneOperation& o,const NinjaVector& input){
    auto& contact=*o.contact;
    if((o.need(o.player.flags_at_10,0x43dcac)&3U)==0&&!parity41(o.fp.compare_float_memory(o.length(o.player.motion.speed),o.parameters.jog_speed)))return;
    NinjaVector axis;
    const auto cross=cross_product_length_extended(input,o.need(contact.vector_at_48,0x43dcd2),axis,o.fp);
    if(o.fp.compare_float_memory(cross,o.c(0x7dcb2c))==X87Comparison::equal)return;
    if(parity41(o.fp.compare_float_memory(o.length(axis),o.c(0x7e0084))))o.normalize(axis);
    else axis=o.need(contact.direction_at_1c,0x43dd06);
    const NinjaPlane first{o.need(contact.vector_at_54,0x43dd2e),o.need(contact.vector_at_48,0x43dd3b)};
    const NinjaPlane second{o.need(contact.center_at_4,0x43dd4f),axis};NinjaPlane intersection;
    (void)intersect_source_planes(first,second,&intersection,o.runtime.response_constants.plane,o.fp);
    auto normalized=input;if(o.fp.compare_float_memory(o.length(normalized),o.c(0x7dcb2c))!=X87Comparison::equal)o.normalize(normalized);
    if(o.fp.compare_float_memory(o.dot(intersection.normal,normalized),o.c(0x7dcb2c))==X87Comparison::greater){
        intersection.normal.x=o.fp.to_float(o.load(intersection.normal.x).negated());intersection.normal.y=o.fp.to_float(o.load(intersection.normal.y).negated());intersection.normal.z=o.fp.to_float(o.load(intersection.normal.z).negated());
    }
    auto& horizontal=o.need(contact.vector_at_6c,0x43de0f);
    const auto scale=o.length((*o.player.flags_at_10&3U)?o.need(contact.displacement_at_10,0x43de1b):horizontal);
    o.scale_retained(horizontal,intersection.normal,scale);
    if(o.need(o.player.prior_attributes,0x43de3e)&0x800000U){auto& registered=o.registered_motion(0x43de53);const auto speed=o.length(registered.motion.speed);o.scale_retained(registered.motion.speed,intersection.normal,speed);}
    auto old=o.need(contact.displacement_at_10,0x43de7d);
    if(o.fp.compare_float_memory(o.length(old),o.c(0x7dd58c))==X87Comparison::greater){
        normalized=horizontal;if(o.fp.compare_float_memory(o.length(normalized),o.c(0x7dd58c))==X87Comparison::greater){
            o.normalize(old);o.normalize(normalized);o.subtract_angle(o.angle(old,normalized,&o.need(contact.direction_at_1c,0x43deec),0x43defa));
        }
    }
    o.align_motion_yaw(0x43df21);
}
// Actual43df60 has a different cross/angle/speed tail, not a formula alias.
void slide_scene_corner(SceneOperation& o,const NinjaVector& input){
    auto& contact=*o.contact;
    if((o.need(o.player.flags_at_10,0x43df6a)&3U)==0&&!parity41(o.fp.compare_float_memory(o.length(o.player.motion.speed),o.parameters.jog_speed)))return;
    NinjaVector cross;if(o.fp.compare_float_memory(cross_product_length_extended(input,o.need(contact.vector_at_48,0x43df94),cross,o.fp),o.c(0x7dcb2c))==X87Comparison::equal)return;
    auto old=o.need(contact.displacement_at_10,0x43dfad),next=o.need(contact.vector_at_6c,0x43dfc3);
    if(o.fp.compare_float_memory(o.length(next),o.c(0x7dcb2c))==X87Comparison::equal)return;
    o.normalize(old);o.normalize(next);o.subtract_angle(o.angle(old,next,&o.need(contact.direction_at_1c,0x43e00f),0x43e01d));o.align_motion_yaw(0x43e038);
    next=*contact.vector_at_6c;
    if(!carry(o.fp.compare_float_memory(o.length(next),o.c(0x7dd550)))){
        auto& registered=o.registered_motion(0x43e069);const auto retained=o.length(registered.motion.speed);float speed=o.fp.to_float(retained);
        if(o.fp.compare_float_memory(retained,o.c(0x7dd584))==X87Comparison::greater)speed=o.fp.to_float(o.fp.multiply_float_memory(o.load(speed),o.c(0x7e0088)));
        o.normalize(next);registered.motion.speed.x=o.fp.to_float(o.fp.multiply_float_memory(o.load(next.x),speed));registered.motion.speed.y=o.fp.to_float(o.fp.multiply_float_memory(o.load(next.y),speed));registered.motion.speed.z=o.fp.to_float(o.fp.multiply_float_memory(o.load(next.z),speed));
    }
}
NinjaVector negated_world(SceneOperation& o){const auto& speed=o.motion(0x444bd4).motion.speed;return {o.fp.to_float(o.load(speed.x).negated()),o.fp.to_float(o.load(speed.y).negated()),o.fp.to_float(o.load(speed.z).negated())};}
NinjaVector old_position_direction(SceneOperation& o,const NinjaVector& old){const auto& current=o.need(o.task.position,0x444d86);return {
    o.fp.to_float(o.fp.subtract_float_memory(o.load(old.x),current.x)),o.fp.to_float(o.fp.subtract_float_memory(o.load(old.y),current.y)),o.fp.to_float(o.fp.subtract_float_memory(o.load(old.z),current.z))};}
void commit_wall_position(SceneOperation& o,const NinjaVector& offset){
    auto& contact=*o.contact;contact.center_at_4=o.need(contact.vector_at_60,0x444cf4);
    auto& position=o.need(o.task.position,0x444d0f);const auto& center=*contact.center_at_4;
    position.x=o.fp.to_float(o.fp.subtract_float_memory(o.load(center.x),offset.x));position.y=o.fp.to_float(o.fp.subtract_float_memory(o.load(center.y),offset.y));position.z=o.fp.to_float(o.fp.subtract_float_memory(o.load(center.z),offset.z));
    contact.displacement_at_10=o.need(contact.vector_at_6c,0x444d2a);
}
std::int32_t wall_scene_response(SceneOperation& o,const NinjaVector& old,const NinjaVector& offset,std::int32_t accumulator,std::uint32_t attributes){
    auto& contact=*o.contact;o.player.source_vector_at_44=o.need(contact.vector_at_48,0x444ab0);NinjaVector direction;bool slide=false;
    if(o.task.orientation.flags&1U){
        direction=old_position_direction(o,old);
        if(o.fp.compare_float_memory(o.length(direction),o.c(0x7dcb2c))==X87Comparison::equal){commit_wall_position(o,offset);return accumulator;}
        o.normalize(direction);const auto angle=o.angle(direction,*contact.vector_at_48,nullptr,0x444b1b);
        if(attributes&0x200000U){if(angle>0x4000)return accumulator;accumulator=1;slide=true;}
        else if(!carry(o.compare(o.player.motion.speed.x,o.c(0x7e012c)))){
            contact.vector_at_6c=NinjaVector{0,0,0};accumulator=2;o.player.motion.speed={0,0,0};
        }else if(angle<=0x1000){if(!carry(o.compare(o.player.motion.speed.x,o.parameters.crash_speed)))accumulator=2;else{accumulator=1;slide=true;}}
        else if(angle<=0x4000){accumulator=1;slide=true;}
    }else{
        if(o.need(o.runtime.globals.motion_override_at_3b42e70,0x444b9f)){
            direction={-1,0,0};o.coordinate(o.coordinates.task_to_world(&o.task.orientation,direction,o.fp),0x444bc6);
        }else direction=negated_world(o);
        if(o.fp.compare_float_memory(o.length(direction),o.c(0x7dcb2c))!=X87Comparison::equal){
            o.normalize(direction);const auto angle=o.angle(direction,*contact.vector_at_48,nullptr,0x444c1d);const auto dot=o.dot(direction,*contact.vector_at_48);
            if(attributes&0x200000U){if(angle>0x4000)return accumulator;accumulator=1;slide=true;}
            else {
                const bool slow=carry(o.compare(o.player.motion.speed.x,o.c(0x7e012c)));
                const auto dot_cmp=o.fp.compare_float_memory(dot,o.c(0x7dcc74));
                if(!slow&&!carry(dot_cmp)){contact.vector_at_6c=NinjaVector{0,0,0};accumulator=2;o.player.motion.speed={0,0,0};}
                else if(slow&&!carry(dot_cmp)){if(!carry(o.compare(o.player.motion.speed.x,o.parameters.crash_speed)))accumulator=2;else accumulator=1;}
                else if(o.task.character_id==3&&angle<=0x3400&&!(attributes&0x201000U))accumulator=1;
                else if(angle>=0x4000)return accumulator;
                else slide=true;
            }
        }
    }
    if(slide)slide_scene_wall(o,direction);commit_wall_position(o,offset);return accumulator;
}
std::int32_t corner_scene_response(SceneOperation& o,const NinjaVector& old,const NinjaVector& offset,std::int32_t accumulator,std::uint32_t attributes){
    auto& contact=*o.contact;o.player.source_vector_at_44=o.need(contact.vector_at_48,0x444d61);NinjaVector direction;
    if(o.task.orientation.flags&1U){direction=old_position_direction(o,old);if(o.fp.compare_float_memory(o.length(direction),o.c(0x7dcb2c))==X87Comparison::equal){direction=negated_world(o);if(o.fp.compare_float_memory(o.length(direction),o.c(0x7dcb2c))==X87Comparison::equal)return accumulator;}
        o.normalize(direction);if(o.angle(direction,*contact.vector_at_48,nullptr,0x444e02)>=0x4000)return accumulator;accumulator=1;
    }else {direction=negated_world(o);if(o.fp.compare_float_memory(o.length(direction),o.c(0x7dcb2c))==X87Comparison::equal){commit_wall_position(o,offset);return accumulator;}
        o.normalize(direction);const auto angle=o.angle(direction,*contact.vector_at_48,nullptr,0x444e69);(void)o.dot(direction,*contact.vector_at_48);
        if(attributes&0x200000U){if(angle>=0x4000)return accumulator;accumulator=1;}
        else if(angle>=0x4000)return accumulator;
    }
    slide_scene_corner(o,direction);commit_wall_position(o,offset);return accumulator;
}
NormalAnglesProvider normal_owner(SceneOperation& o){return [&o](const NinjaVector& normal,X87Context& fp){
    o.result.floor_angles=floor_normal_angles(normal,o.runtime.floor_constants,o.runtime.query_math.inverse_math,fp);
    const auto& result=*o.result.floor_angles;
    return NormalAnglesResult{result.failure==CrtAsinFailure::none&&result.angles?CoordinateFailure::none:CoordinateFailure::normal_angles_unavailable,result.angles};
};}
void floor_transform(SceneOperation& o,const NinjaVector& normal,NinjaVector& value,std::uint32_t site){
    const auto transformed=o.coordinates.floor_to_world(normal,value,o.task.orientation.angles[1],normal_owner(o),o.fp);
    o.coordinate(transformed,site);
}
void contact_offsets(SceneOperation& o,bool alternate,bool use_floor,const NinjaVector* floor,std::optional<float> initial_half={}){
    auto& contact=*o.contact;
    NinjaVector half{0,0,0},center{0,0,0};
    if(alternate){
        o.coordinates.begin_unit_frame();auto& matrix=o.coordinates.current_matrix();
        if(o.task.orientation.angles[2])o.coordinates.rotate_z(matrix,static_cast<std::uint16_t>(o.task.orientation.angles[2]),o.fp);
        if(o.task.orientation.angles[0])o.coordinates.rotate_x(matrix,static_cast<std::uint16_t>(o.task.orientation.angles[0]),o.fp);
        if(o.task.orientation.angles[1])o.coordinates.rotate_y(matrix,static_cast<std::uint16_t>(0U-static_cast<std::uint32_t>(o.task.orientation.angles[1])),o.fp);
        contact.angle_x_at_34=o.task.orientation.angles[0];contact.angle_z_at_38=o.task.orientation.angles[2];
        const NinjaVector vertical{0,1,0};NinjaVector direction;
        o.coordinate(o.coordinates.calculate_vector(matrix,vertical,direction,o.fp),0x445dbb);contact.direction_at_1c=direction;
        half.y=o.fp.to_float(o.fp.multiply_float_memory(o.load(o.parameters.height),o.c(0x7dcc98)));center=half;
        NinjaVector transformed_half;o.coordinate(o.coordinates.calculate_vector(matrix,half,transformed_half,o.fp),0x445df2);half=transformed_half;
        o.result.initialization=initialize_player_contact_offset(center,o.motion(0x445e03),o.player,contact,o.parameters,o.player.flags_at_10,
            o.runtime.response_math.world_gravity,o.runtime.initializer_constants,o.runtime.rotation_constants,o.fp);
        if(!*o.result.initialization)o.stop(ScenePositionFailure::initializer_failed,0x445e03,o.result.initialization->source_boundary);
        NinjaVector transformed_center;o.coordinate(o.coordinates.calculate_vector(matrix,center,transformed_center,o.fp),0x445e16);center=transformed_center;o.coordinates.end_frame();
    }else {
        if(!initial_half){contact.direction_at_1c=NinjaVector{0,1,0};contact.angle_x_at_34=o.task.orientation.angles[0];contact.angle_z_at_38=o.task.orientation.angles[2];}
        half.y=initial_half?*initial_half:o.fp.to_float(o.fp.multiply_float_memory(o.load(o.parameters.height),o.c(0x7dcc98)));center=half;
        if(use_floor){floor_transform(o,*floor,*contact.direction_at_1c,0x4495a2);floor_transform(o,*floor,half,0x4495b1);}
        else {o.coordinate(o.coordinates.task_to_world(&o.task.orientation,*contact.direction_at_1c,o.fp),0x4495e0);o.coordinate(o.coordinates.task_to_world(&o.task.orientation,half,o.fp),0x4495eb);}
        o.result.initialization=initialize_player_contact_offset(center,o.motion(use_floor?0x4495c2:0x449603),o.player,contact,o.parameters,o.player.flags_at_10,
            o.runtime.response_math.world_gravity,o.runtime.initializer_constants,o.runtime.rotation_constants,o.fp);
        if(!*o.result.initialization)o.stop(ScenePositionFailure::initializer_failed,use_floor?0x4495c2:0x449603,o.result.initialization->source_boundary);
        if(use_floor)floor_transform(o,*floor,center,0x4495d4);else o.coordinate(o.coordinates.task_to_world(&o.task.orientation,center,o.fp),0x44960e);
    }
    o.result.half_height_offset=half;o.result.center_offset=center;
}
void reset_sweep(SceneOperation& o,std::uint32_t site){
    auto& center=o.need(o.contact->center_at_4,site);const auto& offset=o.need(o.result.center_offset,site);const auto& position=o.need(o.task.position,site);
    center.x=o.fp.to_float(o.fp.add_float_memory(o.load(offset.x),position.x));center.y=o.fp.to_float(o.fp.add_float_memory(o.load(offset.y),position.y));center.z=o.fp.to_float(o.fp.add_float_memory(o.load(offset.z),position.z));
    o.contact->displacement_at_10=o.motion(site).motion.speed;
}
void add_displacement(SceneOperation& o,bool direct_world=false){
    auto& position=o.need(o.task.position,0x4497e7);const auto& displacement=direct_world?o.motion(0x4497ea).motion.speed:o.need(o.contact->displacement_at_10,0x44a77c);
    position.x=o.fp.to_float(o.fp.add_float_memory(o.load(position.x),displacement.x));position.y=o.fp.to_float(o.fp.add_float_memory(o.load(displacement.y),position.y));position.z=o.fp.to_float(o.fp.add_float_memory(o.load(displacement.z),position.z));
}
bool entry_allowed(SceneOperation& o,const SceneLandEntry& entry,bool alternate){
    const auto attributes=o.need(entry.attributes,alternate?0x445ebe:0x4496d3);
    if((attributes&0x400002U)&&o.task.character_id!=6)return false;
    const auto task_ref=o.need(entry.task,alternate?0x445ed5:0x4496e7);if(!task_ref.id)return true;
    if(!o.runtime.geometry_tasks)o.stop(ScenePositionFailure::unavailable_geometry_task,alternate?0x445edd:0x4496ee);
    const auto found=o.runtime.geometry_tasks->find(*task_ref.id);
    if(found==o.runtime.geometry_tasks->end()||!found->second)o.stop(ScenePositionFailure::unavailable_geometry_task,alternate?0x445edd:0x4496ee);
    const auto& owner=*found->second;
    if(!owner.primary_work){if(alternate)o.stop(ScenePositionFailure::null_geometry_primary,0x445ee3);return true;}
    if(!owner.primary_state)o.stop(ScenePositionFailure::unavailable_live_primary,alternate?0x445ee3:0x4496f7);
    return (owner.primary_state->orientation.flags&0x100U)!=0;
}
void first_scene_pass(SceneOperation& o,bool alternate){
    const auto capacity=alternate?32U:64U;
    for(std::int32_t index=0;index<o.scene.candidate_count;++index){o.result.candidate_index=static_cast<std::size_t>(index);
        const auto& entry=o.need(o.scene.candidates[static_cast<std::size_t>(index)],alternate?0x445ebe:0x4496d3);
        if(!entry_allowed(o,entry,alternate))continue;
        const auto attributes=*entry.attributes;
        if(o.query(entry,alternate?0x445f0a:0x449710)){
            const auto grounded=(o.need(o.player.flags_at_10,alternate?0x445f1c:0x449723)&3U)!=0;
            const auto source=grounded?(alternate?PlayerContactResponseEntry::at_43cf90:PlayerContactResponseEntry::at_43cce0):PlayerContactResponseEntry::at_43d180;
            const auto response=o.respond(source,attributes,grounded?(alternate?0x445f39:0x449740):(alternate?0x445f46:0x449762));
            if(response==1)o.result.winning_index=o.result.processed_count;
        }
        if(o.result.processed_count>=capacity)o.stop(ScenePositionFailure::source_local_buffer_overflow,alternate?0x445f66:0x44978b);
        o.result.first_pass[o.result.processed_count++]=entry;
        if(!alternate&&(attributes&0x4000U))o.runtime.globals.special_surface_at_3b42e74=1;
    }
}
void order_scene_pass(SceneOperation& o,bool alternate){
    const auto count=o.result.processed_count;std::size_t cursor=0;
    if(o.result.winning_index)o.result.ordered[cursor++]=o.result.first_pass[*o.result.winning_index];
    const bool forward=static_cast<bool>(o.need(o.player.moving_task_at_6c,alternate?0x446028:0x44987a));
    for(std::size_t ordinal=0;ordinal<count;++ordinal){const auto index=forward?ordinal:count-1-ordinal;
        if(o.result.winning_index&&index==*o.result.winning_index)continue;
        const auto& entry=o.need(o.result.first_pass[index],alternate?0x44604d:0x4498b0);
        if(o.need(o.player.cached_land_object_at_74,alternate?0x446076:0x4498d6).id==o.need(entry.object,alternate?0x446076:0x4498d6).id)o.result.ordered[count-1]=entry;
        else {if(cursor>=count)o.stop(ScenePositionFailure::source_local_buffer_overflow,alternate?0x4460ab:0x449914);o.result.ordered[cursor++]=entry;}
    }
}
void airborne_scene_tail(SceneOperation& o,bool alternate){
    bool back=false;const auto flags=o.need(o.player.flags_at_10,alternate?0x4467ca:0x44a6a0);
    if((flags&1U)&&!(flags&0x4000U)){
        float magnitude=0;o.result.input=get_player_input(input_task_view(o.task),o.runtime.input,o.fp,nullptr,&magnitude,o.runtime.forced_angle);
        if(!*o.result.input||!o.result.input->active)o.stop(ScenePositionFailure::unavailable_field,alternate?0x4467f5:0x44a6d2,o.result.input->source_boundary);
        bool skip=false;
        if(*o.result.input->active){skip=parity41(o.compare(magnitude,o.c(0x7dfff0)));if(!skip)skip=o.need(o.player.no_control_time_at_a,alternate?0x446812:0x44a6f3)!=0;}
        if(!skip&&!parity41(o.compare(o.player.motion.speed.y,o.c(0x7dcb2c)))&&!parity41(o.compare(o.player.motion.speed.x,o.parameters.run_speed))){
            if((alternate||!(o.need(o.player.prior_attributes,0x44a71d)&0x1cU))&&o.compare(o.player.dotp,o.c(0x7dd578))==X87Comparison::greater)back=true;
        }
    }
    if(back){auto& position=o.need(o.task.position,alternate?0x446858:0x44a743);auto& speed=o.motion(alternate?0x446852:0x44a73a).motion.speed;
        position.x=o.fp.to_float(o.fp.reverse_subtract_float_memory(o.fp.multiply_float_memory(o.load(speed.x),o.c(0x7dcc98)),position.x));
        position.y=o.fp.to_float(o.fp.reverse_subtract_float_memory(o.fp.multiply_float_memory(o.load(speed.y),o.c(0x7dcc98)),position.y));
        position.z=o.fp.to_float(o.fp.reverse_subtract_float_memory(o.fp.multiply_float_memory(o.load(speed.z),o.c(0x7dcc98)),position.z));speed={0,0,0};
    }else add_displacement(o);
    o.attributes(0,alternate?0x4468b9:0x44a7ab);
}
bool scene_contact_gate(SceneOperation& o,const SceneLandEntry& entry){
    auto& contact=*o.contact;const auto backup=contact;
    const auto registry=o.runtime.platform.players;
    if(!registry||o.task.player_index>=registry->player_work.size())o.stop(ScenePositionFailure::unavailable_registry_owner,0x444661);
    const auto allocation=o.need(registry->player_work[o.task.player_index],0x444661);
    if(!allocation||!allocation->player||!allocation->parameters)o.stop(ScenePositionFailure::unavailable_registry_owner,0x444668);
    auto& registered=*allocation->player;const auto& p=*allocation->parameters;
    const auto special=o.need(o.runtime.globals.special_surface_at_3b42e74,0x44467a);
    NinjaVector center{0,4,0};if(special&&o.task.player_index==5)center.y=o.need(o.runtime.globals.amy_offset_at_915d74,0x4446a1);
    o.result.initialization=initialize_player_contact_offset(center,o.registered_motion(0x44465b),registered,contact,p,registered.flags_at_10,
        o.runtime.response_math.world_gravity,o.runtime.initializer_constants,o.runtime.rotation_constants,o.fp);
    if(!*o.result.initialization)o.stop(ScenePositionFailure::initializer_failed,0x4446b4,o.result.initialization->source_boundary);
    const bool floor=special&&!(o.task.orientation.flags&0x2000U);
    if(floor)floor_transform(o,o.need(contact.vector_at_48,0x4446e3),center,0x4446e7);
    else o.coordinate(o.coordinates.task_to_world(&o.task.orientation,center,o.fp),0x4446f7);
    NinjaVector radius_offset{0,o.need(contact.radius_at_0,0x444705),0};
    if(special&&o.task.player_index==5)radius_offset.y=o.need(o.runtime.globals.amy_radius_at_915d78,0x444723);
    if(floor)floor_transform(o,*contact.vector_at_48,radius_offset,0x44474a);
    else o.coordinate(o.coordinates.task_to_world(&o.task.orientation,radius_offset,o.fp),0x44475a);
    const auto attributes=o.need(entry.attributes,0x444785);auto& point=o.need(contact.center_at_4,0x4447da);
    const auto add_offset=[&](const NinjaVector& base){point.x=o.fp.to_float(o.fp.add_float_memory(o.load(center.x),base.x));point.y=o.fp.to_float(o.fp.add_float_memory(o.load(center.y),base.y));point.z=o.fp.to_float(o.fp.add_float_memory(o.load(center.z),base.z));};
    if(floor){
        const auto& plane=o.need(contact.vector_at_60,0x4447c6);
        if(attributes&0x4000U){contact.displacement_at_10=NinjaVector{0,0,0};const auto& horizontal=o.need(contact.vector_at_6c,0x44478c);
            point.x=o.fp.to_float(o.fp.add_float_memory(o.fp.subtract_float_memory(o.fp.add_float_memory(o.load(horizontal.x),plane.x),radius_offset.x),center.x));
            point.y=o.fp.to_float(o.fp.add_float_memory(o.fp.subtract_float_memory(o.fp.add_float_memory(o.load(horizontal.y),plane.y),radius_offset.y),center.y));
            point.z=o.fp.to_float(o.fp.add_float_memory(o.fp.subtract_float_memory(o.fp.add_float_memory(o.load(horizontal.z),plane.z),radius_offset.z),center.z));
        }else {
            point.x=o.fp.to_float(o.fp.add_float_memory(o.fp.subtract_float_memory(o.load(plane.x),radius_offset.x),center.x));point.y=o.fp.to_float(o.fp.add_float_memory(o.fp.subtract_float_memory(o.load(plane.y),radius_offset.y),center.y));point.z=o.fp.to_float(o.fp.add_float_memory(o.fp.subtract_float_memory(o.load(plane.z),radius_offset.z),center.z));
            if(o.angle(o.need(contact.direction_at_1c,0x4447d2),o.need(contact.vector_at_48,0x4447ce),nullptr,0x4447f9)>=0x2c00){
                auto horizontal=o.need(contact.vector_at_6c,0x44480c);
                if(!carry(o.fp.compare_float_memory(o.length(horizontal),o.c(0x7dcb10)))){o.normalize(horizontal);o.scale_retained(o.need(contact.displacement_at_10,0x44484f),horizontal,o.length(*contact.displacement_at_10));}
            }
        }
    }else if(o.task.player_index==7)add_offset(o.need(contact.vector_at_54,0x44488d));
    else {
        const auto old=point;const auto& plane=o.need(contact.vector_at_60,0x444895);const auto& position=o.need(o.task.position,0x4448a4);
        point.x=o.fp.to_float(o.fp.add_float_memory(o.fp.add_float_memory(o.fp.subtract_float_memory(o.load(plane.x),old.x),position.x),center.x));
        point.y=o.fp.to_float(o.fp.add_float_memory(o.fp.add_float_memory(o.fp.subtract_float_memory(o.load(plane.y),old.y),position.y),center.y));
        point.z=o.fp.to_float(o.fp.add_float_memory(o.fp.add_float_memory(o.fp.subtract_float_memory(o.load(plane.z),old.z),position.z),center.z));
    }
    const auto high=o.compare(registered.motion.speed.x,p.dash_speed);
    if(!carry(high)||(o.need(registered.prior_attributes,0x444901)&0x4000U)||
       (!parity41(o.compare(registered.motion.speed.y,o.c(0x7dcb2c)))&&(attributes&0x4000U)))contact.radius_at_0=5.1F;
    else if(carry(o.compare(registered.motion.speed.x,p.jog_speed)))contact.radius_at_0=4.1F;
    else {
        const auto speed=o.fp.subtract_float_memory(o.load(registered.motion.speed.x),p.jog_speed);
        const auto range=o.fp.subtract_float_memory(o.load(p.dash_speed),p.jog_speed);
        contact.radius_at_0=o.fp.to_float(o.fp.add_float_memory(o.fp.divide(speed,range),o.c(0x7e0128)));
    }
    if(o.task.player_index==7)contact.radius_at_0=o.fp.to_float(o.fp.add_float_memory(o.load(*contact.radius_at_0),o.c(0x7dd540)));
    if(o.need(o.runtime.globals.render_at_3b0f0ec,0x444967)>=0){
        o.coordinates.push_current_frame();o.coordinates.translate_matrix(o.coordinates.current_matrix(),point,o.fp);
        o.stop(ScenePositionFailure::render_owner_unavailable,0x44499e); // actual4128a0 color/draw state is not reconstructed
    }
    if(o.query(entry,0x4449e3)){
        const auto normal_angle=o.angle(o.need(contact.direction_at_1c,0x4449f4),o.need(contact.vector_at_48,0x4449f1),nullptr,0x4449f9);
        bool accept=true;
        if(normal_angle>0x2c00){const auto& gravity=o.need(o.runtime.response_math.world_gravity,0x444a0a);
            const float old_dot=o.fp.to_float(o.dot(gravity,*contact.direction_at_1c));const auto comparison=o.fp.compare_float_memory(o.dot(gravity,*contact.vector_at_48),old_dot);
            if(comparison!=X87Comparison::less&&!(attributes&0x4000U))accept=false;
        }
        if(accept){const auto& gravity=o.need(o.runtime.response_math.world_gravity,0x444a3b);
            if(o.fp.compare_float_memory(o.dot(gravity,*contact.vector_at_48),o.c(0x7dcb2c))!=X87Comparison::greater||
               (o.need(registered.flags_at_10,0x444a56)&3U)){
                contact.radius_at_0=backup.radius_at_0;contact.displacement_at_10=backup.displacement_at_10;return true;
            }
        }
    }
    contact=backup;return false;
}
void store_floor_angles(SceneOperation& o,const NinjaVector& normal,std::uint32_t site){
    try{
        o.result.floor_angles=floor_normal_angles(normal,o.runtime.floor_constants,o.runtime.query_math.inverse_math,o.fp);
        if(o.result.floor_angles->failure!=CrtAsinFailure::none||!o.result.floor_angles->angles)o.stop(ScenePositionFailure::floor_angles_failed,site,o.result.floor_angles->source_boundary);
        o.contact->angle_x_at_34=o.result.floor_angles->angles->inclination;o.contact->angle_z_at_38=o.result.floor_angles->angles->roll;
    }catch(const FloorAngleArithmeticError& error){o.contact->angle_x_at_34=error.stored_inclination();throw;}
}
TaskHandle geometry_task(SceneOperation& o,const SceneResourceReference& ref,std::uint32_t site){
    if(!ref.id)return {};if(!o.runtime.geometry_tasks)o.stop(ScenePositionFailure::unavailable_geometry_task,site);
    const auto found=o.runtime.geometry_tasks->find(*ref.id);if(found==o.runtime.geometry_tasks->end()||!found->second)o.stop(ScenePositionFailure::unavailable_geometry_task,site);return found->second;
}
void project_foot_position(SceneOperation& o,const SceneResourceReference& moving,bool alternate){
    auto& contact=*o.contact;const auto horizontal=o.need(contact.vector_at_6c,alternate?0x4465bd:0x44a37a);const auto plane=o.need(contact.vector_at_60,alternate?0x4465bd:0x44a37d);
    auto& center=o.need(contact.center_at_4,alternate?0x4465cc:0x44a38a);
    center.x=o.fp.to_float(alternate?o.fp.add_float_memory(o.load(plane.x),horizontal.x):o.fp.add_float_memory(o.load(horizontal.x),plane.x));
    center.y=o.fp.to_float(o.fp.add_float_memory(o.load(horizontal.y),plane.y));center.z=o.fp.to_float(o.fp.add_float_memory(o.load(horizontal.z),plane.z));
    const auto selected=o.need(contact.selected_contact_at_2a,alternate?0x4465c0:0x44a37d);
    if(selected>=o.runtime.contacts.responses.size()||!o.runtime.contacts.responses[selected])o.stop(ScenePositionFailure::unavailable_field,alternate?0x446604:0x44a3c6);
    const auto normal=o.need(contact.direction_at_1c,alternate?0x4465c6:0x44a384);NinjaVector projected;
    if(!alternate)contact.displacement_at_10=NinjaVector{0,0,0};
    (void)project_point_onto_plane(center,o.runtime.contacts.responses[selected]->point_at_20,normal,&projected,o.runtime.response_constants.plane.distance_scale,o.fp);
    const auto& offset=o.need(o.result.half_height_offset,alternate?0x44663b:0x44a410);
    center.x=o.fp.to_float(o.fp.add_float_memory(o.load(projected.x),offset.x));center.y=o.fp.to_float(o.fp.add_float_memory(o.load(projected.y),offset.y));center.z=o.fp.to_float(o.fp.add_float_memory(o.load(projected.z),offset.z));
    o.player.moving_task_at_6c=geometry_task(o,moving,alternate?0x44666c:0x44a43d);
    if(moving.id)o.leaf(move_scene_platform(o.task,o.player,contact,offset,o.runtime.platform,o.runtime.axis_constants,o.fp),alternate?0x44666f:0x44a447);
    const NinjaVector candidate{
        o.fp.to_float(o.fp.subtract_float_memory(o.load(center.x),offset.x)),o.fp.to_float(o.fp.subtract_float_memory(o.load(center.y),offset.y)),o.fp.to_float(o.fp.subtract_float_memory(o.load(center.z),offset.z))};
    const auto& position=o.need(o.task.position,alternate?0x4466b0:0x44a481);
    const auto distance=[&](){if(alternate){
        const auto dy=o.fp.subtract_float_memory(o.load(position.y),candidate.y);const auto dx=o.fp.subtract_float_memory(o.load(position.x),candidate.x);const auto dz=o.fp.subtract_float_memory(o.load(position.z),candidate.z);
        const auto zz=o.fp.multiply(dz,dz),xx=o.fp.multiply(dx,dx);const auto sum=o.fp.add(zz,xx);const auto yy=o.fp.multiply(dy,dy);return signed_sqrt_extended(o.fp.to_float(o.fp.add(sum,yy)),o.fp);
    }else {
        const auto dz=o.fp.subtract_float_memory(o.load(position.z),candidate.z);const auto dy=o.fp.subtract_float_memory(o.load(position.y),candidate.y);const auto dx=o.fp.subtract_float_memory(o.load(position.x),candidate.x);
        const auto xx=o.fp.multiply(dx,dx),yy=o.fp.multiply(dy,dy);const auto sum=o.fp.add(xx,yy);const auto zz=o.fp.multiply(dz,dz);return signed_sqrt_extended(o.fp.to_float(o.fp.add(sum,zz)),o.fp);
    }}();
    const auto override=alternate?0U:o.need(o.runtime.globals.motion_override_at_3b42e70,0x44a4b7);
    const bool changed=moving.id||o.compare(o.player.motion.speed.x,o.c(0x7dcb2c))!=X87Comparison::equal||o.compare(o.player.motion.speed.z,o.c(0x7dcb2c))!=X87Comparison::equal||o.fp.compare_float_memory(distance,o.c(0x7dcb10))==X87Comparison::greater||override;
    if(changed&&(o.fp.compare_float_memory(distance,o.c(0x7e008c))==X87Comparison::greater||override))o.task.position=candidate;
}
void initial_scene(SceneOperation& o,bool alternate){
    o.result.previous_flags=o.need(o.player.flags_at_10,alternate?0x445d1d:0x44950d);
    if(!o.player.contact_work)o.stop(ScenePositionFailure::unavailable_contact_work,alternate?0x445d2e:0x44951c);o.contact=&*o.player.contact_work;
    o.player.flags_at_10=o.task.orientation.flags;o.task.orientation.flags&=0xfffeU;
    bool floor=false;const NinjaVector* normal=nullptr;std::optional<float> initial_half;
    if(!alternate){
        o.contact->direction_at_1c=NinjaVector{0,1,0};o.runtime.globals.special_surface_at_3b42e74=0;
        o.contact->angle_x_at_34=o.task.orientation.angles[0];o.contact->angle_z_at_38=o.task.orientation.angles[2];
        initial_half=o.fp.to_float(o.fp.multiply_float_memory(o.load(o.parameters.height),o.c(0x7dcc98)));
        floor=(o.need(o.player.prior_attributes,0x44958d)&0x4000U)!=0;if(floor)normal=&o.need(o.player.floor_normal,0x44959e);
    }
    contact_offsets(o,alternate,floor,normal,initial_half);reset_sweep(o,alternate?0x445e25:0x449616);
    o.contact->radius_at_0=o.fp.to_float(o.fp.multiply_float_memory(o.load(o.parameters.height),o.c(0x7dcc98)));
    o.result.temporary_vector=o.need(o.task.position,alternate?0x445e75:0x449674);
    o.result.broadphase=query_player_scene_broadphase(o.scene,*o.contact,o.runtime.broadphase_constants,o.coordinates,o.fp);
    if(!*o.result.broadphase)o.stop(ScenePositionFailure::broadphase_failed,alternate?0x445e89:0x449685,o.result.broadphase->source_boundary);
    first_scene_pass(o,alternate);
}
bool early_scene_exit(SceneOperation& o,bool alternate){
    if(o.result.processed_count)return false;
    add_displacement(o,true);o.attributes(0,alternate?0x445fda:0x44980b);o.player.moving_task_at_6c=TaskHandle{};o.player.cached_land_object_at_74=SceneResourceReference{};o.result.outcome=0;return true;
}
void alternate_scene_pass(SceneOperation& o){
    auto& contact=*o.contact;order_scene_pass(o,true);reset_sweep(o,0x446164);o.player.moving_task_at_6c=TaskHandle{};
    bool landed=false;SceneResourceReference cached;std::int32_t accumulator=0;
    for(std::int32_t index=0;index<static_cast<std::int32_t>(o.result.processed_count);++index){
        o.result.response_index=static_cast<std::size_t>(index);auto& entry=o.need(o.result.ordered[static_cast<std::size_t>(index)],0x4461d3);
        if(!o.query(entry,0x4461ea))continue;
        const auto attributes=o.need(entry.attributes,0x44621d);const auto& normal=o.need(contact.vector_at_48,0x4461fc);const auto& direction=o.need(contact.direction_at_1c,0x4461ff);
        const auto slope=o.angle(direction,normal,nullptr,0x446204);bool wall=(o.task.orientation.flags&2U)!=0;
        if(!wall&&(attributes&0x200040U)){
            if(!(attributes&0x200000U)||o.need(o.runtime.globals.environment_at_3b22dec,0x446231)!=3)wall=true;
            else wall=o.fp.compare_float_memory(o.load(o.c(0x7e0118)),normal.y)!=X87Comparison::less;
        }
        if(!wall&&slope>0x2c00)wall=o.fp.compare_float_memory(o.load(direction.y),normal.y)!=X87Comparison::less;
        const auto flags=o.need(o.player.flags_at_10,0x446279);
        if(!wall&&(*o.result.previous_flags&3U)&&!(flags&3U))wall=true;
        if(!wall){const auto& gravity=o.need(o.runtime.response_math.world_gravity,0x446285);wall=o.fp.compare_float_memory(o.dot(gravity,normal),o.c(0x7dcb2c))==X87Comparison::greater&&!(flags&3U);}
        if(!wall){
            const auto vertical=o.compare(o.player.motion.speed.y,o.c(0x7dcb2c));
            if(vertical==X87Comparison::greater||vertical==X87Comparison::unordered){
                const auto again=o.compare(o.player.motion.speed.y,o.c(0x7dcb2c));wall=again!=X87Comparison::greater||!(flags&3U);
            }
        }
        if(wall){accumulator=corner_scene_response(o,*o.result.temporary_vector,*o.result.half_height_offset,accumulator,attributes);o.result.provisional_outcome=accumulator;continue;}
        landed=true;cached=o.need(entry.object,0x44630e);
        const auto returned=o.respond(PlayerContactResponseEntry::at_43d8b0,attributes,0x446323);
        if(returned&&(accumulator==0||returned==2))accumulator=returned;o.result.provisional_outcome=accumulator;
        store_floor_angles(o,normal,0x44635d);
        bool rotate=true;const auto& speed=o.motion(0x446377).motion.speed;
        if(o.compare(speed.x,o.c(0x7dcb2c))==X87Comparison::equal&&o.compare(speed.z,o.c(0x7dcb2c))==X87Comparison::equal&&(o.need(o.player.flags_at_10,0x44638a)&1U)){
            const auto& gravity=o.need(o.runtime.response_math.world_gravity,0x446394);const float old=o.fp.to_float(o.dot(gravity,*contact.direction_at_1c));rotate=o.fp.compare_float_memory(o.dot(gravity,*contact.vector_at_48),old)==X87Comparison::greater;
        }
        if(rotate)o.leaf(rotate_scene_floor(o.task,o.player,o.parameters,contact,o.constants,o.runtime.response_math.world_gravity,o.coordinates,o.fp),0x4463cb);
        if(accumulator==0)contact_offsets(o,true,false,nullptr);
        auto& motion=o.motion(0x4464d4);motion.angle_x_at_1c=o.need(contact.angle_x_at_34,0x4464ce);motion.angle_z_at_24=o.need(contact.angle_z_at_38,0x4464da);o.player.floor_normal=o.need(contact.vector_at_48,0x4464e0);
        auto& horizontal=o.need(contact.vector_at_6c,0x4464f4);
        if(o.fp.compare_float_memory(o.length(horizontal),o.c(0x7e0128))==X87Comparison::greater){
            o.normalize(horizontal);const auto excess=o.fp.subtract_float_memory(o.length(o.need(contact.displacement_at_10,0x44651a)),o.c(0x7e0128));
            auto& displacement=*contact.displacement_at_10;
            displacement.x=o.fp.to_float(o.fp.multiply_float_memory(excess,horizontal.x));displacement.y=o.fp.to_float(o.fp.multiply_float_memory(excess,horizontal.y));displacement.z=o.fp.to_float(o.fp.multiply_float_memory(excess,horizontal.z));
            horizontal.x=o.fp.to_float(o.fp.multiply_float_memory(o.load(horizontal.x),o.c(0x7e0128)));horizontal.y=o.fp.to_float(o.fp.multiply_float_memory(o.load(horizontal.y),o.c(0x7e0128)));horizontal.z=o.fp.to_float(o.fp.multiply_float_memory(o.load(horizontal.z),o.c(0x7e0128)));
            std::swap(o.result.ordered[static_cast<std::size_t>(index)],o.result.ordered[o.result.processed_count-1]);index=-1;
        }else contact.displacement_at_10=NinjaVector{0,0,0};
        project_foot_position(o,o.need(entry.task,0x446657),true);o.task.orientation.flags|=1U;o.attributes(o.need(entry.attributes,0x44674f),0x44675d);
    }
    if(landed){o.player.cached_land_object_at_74=cached;add_displacement(o);o.result.outcome=accumulator;}
    else {airborne_scene_tail(o,true);o.result.outcome=accumulator;}
}
void normalize_scene_inline(SceneOperation& o,NinjaVector& vector){
    const auto z=o.fp.multiply_float_memory(o.load(vector.z),vector.z);const auto y=o.fp.multiply_float_memory(o.load(vector.y),vector.y);const auto zy=o.fp.add(z,y);const auto x=o.fp.multiply_float_memory(o.load(vector.x),vector.x);
    const auto length=o.fp.sqrt(o.fp.add(zy,x));
    if(o.fp.compare_float_memory(length,o.c(0x7dcb2c))==X87Comparison::equal){vector.z=0;vector.y=0;vector.x=0;return;}
    const auto inverse=o.fp.reverse_divide_float_memory(length,o.c(0x7dcb10));
    vector.x=o.fp.to_float(o.fp.multiply(o.load(vector.x),inverse));vector.y=o.fp.to_float(o.fp.multiply(o.load(vector.y),inverse));vector.z=o.fp.to_float(o.fp.multiply_float_memory(inverse,vector.z));
}
bool skip_floor_turn(SceneOperation& o){
    const auto& velocity=o.motion(0x44a02f).motion.speed;
    if(o.compare(velocity.x,o.c(0x7dcb2c))!=X87Comparison::equal)return false;
    if(o.compare(velocity.z,o.c(0x7dcb2c))!=X87Comparison::equal)return false;
    if(!(o.need(o.player.flags_at_10,0x44a056)&1U))return false;
    const auto& gravity=o.need(o.runtime.response_math.world_gravity,0x44a05e);
    const float normal_dot=o.fp.to_float(o.dot(gravity,o.need(o.contact->vector_at_48,0x44a063)));
    return o.fp.compare_float_memory(o.dot(gravity,o.need(o.contact->direction_at_1c,0x44a06c)),normal_dot)!=X87Comparison::greater;
}
std::int32_t asin_scene_angle(SceneOperation& o,float component,std::uint32_t factor,std::uint32_t site){
    o.result.asin=crt_asin(o.load(component),o.runtime.query_math.inverse_math,o.fp);
    if(o.result.asin->failure!=CrtAsinFailure::none||!o.result.asin->value)o.stop(ScenePositionFailure::asin_failed,site,o.result.asin->source_boundary);
    return signed_bits(crt_ftol_low32(o.fp.multiply_double_memory(o.fp.multiply_double_memory(*o.result.asin->value,o.constants.wide(0x7dccd0)),o.constants.wide(factor)),o.fp));
}
void ordinary_floor_turn(SceneOperation& o,std::uint32_t attributes){
    auto& contact=*o.contact;const auto& normal=o.need(contact.vector_at_48,0x449fbf);
    if(attributes&0x4000U){o.task.orientation.angles[2]=0;o.task.orientation.angles[0]=0;return;}
    if((attributes&0x2001cU)||(o.need(o.player.flags_at_10,0x449fac)&0x2000U)){
        store_floor_angles(o,normal,0x44a14b);if(skip_floor_turn(o))return;
    }else {
        contact.angle_x_at_34=asin_scene_angle(o,normal.z,0x7dccc8,0x449fc2);
        const auto sign=o.compare(normal.y,o.c(0x7dcb2c));const auto z=asin_scene_angle(o,normal.x,0x7dcce8,carry(sign)?0x44a008:0x449fed);
        contact.angle_z_at_38=carry(sign)?signed_bits(0x8000U-static_cast<std::uint32_t>(z)):z;
        if(skip_floor_turn(o))return;
        const auto x=asin_scene_angle(o,normal.z,0x7dccc8,0x44a08b);const auto second=asin_scene_angle(o,normal.x,0x7dcce8,0x44a0a7);
        if(o.compare(normal.y,o.c(0x7dcb2c))==X87Comparison::less){
            const auto reflected_x=signed_bits(0x8000U-static_cast<std::uint32_t>(x));const auto reflected_z=signed_bits(0x8000U-static_cast<std::uint32_t>(second));
            if(angle_distance(o.task.orientation.angles[0],reflected_x)<angle_distance(o.task.orientation.angles[2],reflected_z)){contact.angle_x_at_34=reflected_x;contact.angle_z_at_38=second;}
            else {contact.angle_x_at_34=x;contact.angle_z_at_38=reflected_z;}
        }
    }
    o.leaf(rotate_scene_floor(o.task,o.player,o.parameters,contact,o.constants,o.runtime.response_math.world_gravity,o.coordinates,o.fp),0x44a1c0);
}
void ordinary_landing_flags(SceneOperation& o,std::uint32_t attributes){
    const auto speed=o.player.motion.speed.x;
    bool ground=!carry(o.compare(speed,o.c(0x7e0184)))||!parity41(o.compare(speed,o.c(0x7e0180)));
    const auto& gravity=o.need(o.runtime.response_math.world_gravity,0x44a54d);const auto& direction=o.need(o.contact->direction_at_1c,0x44a548);
    if(!ground)ground=o.fp.compare_float_memory(o.dot(gravity,direction),o.c(0x7e017c))==X87Comparison::less;
    if(!ground)ground=(attributes&0x4000U)!=0;
    if(!ground){
        const auto upper=o.compare(speed,o.c(0x7e0178));
        bool ordinary=carry(upper)&&parity41(o.compare(speed,o.c(0x7e0174)));
        if(!ordinary)ordinary=o.fp.compare_float_memory(o.dot(gravity,direction),o.c(0x7e017c))!=X87Comparison::less;
        if(ordinary){o.attributes(0,0x44a5f2);
            if(o.compare(o.player.motion.speed.x,o.c(0x7dcb2c))==X87Comparison::greater)o.leaf(nudge_scene_normal(o.task,o.world,*o.contact,o.constants,o.fp),0x44a60d);
            return;
        }
        ground=true;
    }
    if(ground){o.task.orientation.flags|=1U;o.attributes(attributes,0x44a5cf);}
}
void ordinary_scene_pass(SceneOperation& o){
    auto& contact=*o.contact;
    if(o.need(o.runtime.globals.event_task_at_3b2c578,0x42fb00))o.runtime.globals.special_surface_at_3b42e74=0;
    order_scene_pass(o,false);reset_sweep(o,0x4499b2);o.player.moving_task_at_6c=TaskHandle{};
    bool landed=false;SceneResourceReference cached,last_object;std::int32_t accumulator=0;
    for(std::size_t index=0;index<o.result.processed_count;++index){o.result.response_index=index;auto& entry=o.need(o.result.ordered[index],0x449a10);
        if(!o.query(entry,0x449a3e))continue;
        const auto attributes=o.need(entry.attributes,0x449da9);const auto object=o.need(entry.object,0x449a6d);const auto special=o.need(o.runtime.globals.special_surface_at_3b42e74,0x449a76);
        std::uint32_t decisions=1;
        if((o.task.orientation.flags&1U)&&last_object.id!=object.id){
            const auto& horizontal=o.need(contact.vector_at_6c,0x449a7c);const auto& plane=o.need(special==1?contact.vector_at_54:contact.vector_at_60,0x449a83);const auto& position=o.need(o.task.position,0x449a86);const auto& half=*o.result.half_height_offset;
            const auto component=[&](float a,float b,float h,float p){auto value=o.fp.add_float_memory(o.load(a),b);if(special!=1)value=o.fp.subtract_float_memory(value,h);return o.fp.to_float(o.fp.subtract_float_memory(value,p));};
            NinjaVector delta{component(horizontal.x,plane.x,half.x,position.x),component(horizontal.y,plane.y,half.y,position.y),component(horizontal.z,plane.z,half.z,position.z)};
            const auto length=o.length(delta);const auto spill=o.fp.to_float(length);
            if(o.fp.compare_float_memory(length,o.c(0x7dcb2c))!=X87Comparison::equal){normalize_scene_inline(o,delta);
                if(o.fp.compare_float_memory(o.dot(delta,o.need(contact.vector_at_48,0x449b59)),o.c(0x7dd570))==X87Comparison::less||o.compare(spill,o.parameters.height)==X87Comparison::greater)decisions=0;
            }
        }
        decisions|=4U;
        if((o.task.orientation.flags&3U)&&!(o.task.orientation.flags&0x2000U)){
            const auto& horizontal=o.need(contact.vector_at_6c,0x449bb9);const auto& plane=o.need(contact.vector_at_54,0x449bc3);const auto& center=o.need(contact.center_at_4,0x449bc6);
            NinjaVector delta{
                o.fp.to_float(o.fp.subtract_float_memory(o.fp.add_float_memory(o.load(horizontal.x),plane.x),center.x)),
                special?0:o.fp.to_float(o.fp.subtract_float_memory(o.fp.add_float_memory(o.load(horizontal.y),plane.y),center.y)),
                o.fp.to_float(o.fp.subtract_float_memory(o.fp.add_float_memory(o.load(horizontal.z),plane.z),center.z))};
            if(o.fp.compare_float_memory(o.length(delta),o.c(0x7dcb2c))!=X87Comparison::equal){
                normalize_scene_inline(o,delta);NinjaVector forward{carry(o.compare(o.player.motion.speed.x,o.c(0x7dcb2c)))?-1.0F:1.0F,0,0};
                o.coordinate(o.coordinates.task_to_world(&o.task.orientation,forward,o.fp),0x449cb3);
                const auto loaded_y=o.load(forward.y);const auto absolute=loaded_y.negative()?loaded_y.negated():loaded_y;
                if(!special||o.fp.compare_float_memory(absolute,o.c(0x7dcb10))!=X87Comparison::equal){normalize_scene_inline(o,forward);
                    if(o.fp.compare_float_memory(o.dot(delta,forward),o.c(0x7e00e4))==X87Comparison::less)decisions&=~4U;
                }
            }
        }
        auto slope=o.angle(o.need(contact.direction_at_1c,0x449d68),o.need(contact.vector_at_48,0x449d65),nullptr,0x449d6d);const bool steep=slope>0x2c00;
        if(steep){(void)scene_contact_gate(o,entry);slope=o.angle(*contact.direction_at_1c,*contact.vector_at_48,nullptr,0x449d8c);decisions|=2U;}
        bool wall=(o.task.orientation.flags&2U)||(attributes&0x200040U);
        if(!wall&&(slope>0x2c00||(decisions&5U)!=5U)){
            const auto& gravity=o.need(o.runtime.response_math.world_gravity,0x449dcb);const auto old=o.fp.to_float(o.dot(gravity,*contact.direction_at_1c));
            if(o.fp.compare_float_memory(o.dot(gravity,*contact.vector_at_48),old)!=X87Comparison::less)wall=!(attributes&0x4000U)||(decisions&5U)!=5U;
        }
        const auto flags=o.need(o.player.flags_at_10,0x449e18);
        if(!wall&&(*o.result.previous_flags&3U)&&!(flags&3U))wall=true;
        if(!wall){const auto& gravity=o.need(o.runtime.response_math.world_gravity,0x449e24);wall=o.fp.compare_float_memory(o.dot(gravity,*contact.vector_at_48),o.c(0x7dcb2c))==X87Comparison::greater&&!(flags&3U);}
        if(!wall&&!steep&&!scene_contact_gate(o,entry))wall=true;
        if(!wall&&o.task.character_id==7&&o.need(o.task.action_at_0,0x449e6d)==0x30)wall=true;
        if(!wall&&!carry(o.fp.compare_float_memory(o.length(o.motion(0x449e7d).motion.speed),o.parameters.run_speed))&&!(attributes&0x4000U)){
            auto velocity=o.world->motion.speed;o.normalize(velocity);wall=o.fp.compare_float_memory(o.dot(velocity,*contact.vector_at_48),o.c(0x7e0188))==X87Comparison::greater;
        }
        if(!wall){const auto vertical=o.compare(o.player.motion.speed.y,o.c(0x7dcb2c));
            if(vertical==X87Comparison::greater||vertical==X87Comparison::unordered){const auto again=o.compare(o.player.motion.speed.y,o.c(0x7dcb2c));wall=again!=X87Comparison::greater||!(flags&3U);}
        }
        if(wall){accumulator=wall_scene_response(o,*o.result.temporary_vector,*o.result.half_height_offset,accumulator,attributes);o.result.provisional_outcome=accumulator;continue;}
        landed=true;cached=object;last_object=object;const auto returned=o.respond(PlayerContactResponseEntry::at_43d360,attributes,0x449f5d);
        if(returned&&(accumulator==0||returned==2))accumulator=returned;o.result.provisional_outcome=accumulator;ordinary_floor_turn(o,attributes);
        if(accumulator==0){const bool floor=(attributes&0x4000U)!=0;const auto* normal=floor?&o.need(contact.vector_at_48,0x44a204):nullptr;contact_offsets(o,false,floor,normal);}
        auto& motion=o.motion(0x44a2da);motion.angle_x_at_1c=o.need(contact.angle_x_at_34,0x44a2d7);motion.angle_z_at_24=o.need(contact.angle_z_at_38,0x44a2e4);o.player.floor_normal=o.need(contact.vector_at_48,0x44a2f1);
        if(attributes&0x4000U){auto horizontal=o.need(contact.vector_at_6c,0x44a313);
            if(o.fp.compare_float_memory(o.length(horizontal),o.c(0x7dcb2c))!=X87Comparison::equal){const float length=o.fp.to_float(o.length(o.need(contact.displacement_at_10,0x44a33f)));o.normalize(horizontal);
                auto& target=*contact.vector_at_6c;target.x=o.fp.to_float(o.fp.multiply_float_memory(o.load(length),horizontal.x));target.y=o.fp.to_float(o.fp.multiply_float_memory(o.load(length),horizontal.y));target.z=o.fp.to_float(o.fp.multiply_float_memory(o.load(length),horizontal.z));}
        }
        project_foot_position(o,o.need(entry.task,0x44a432),false);ordinary_landing_flags(o,attributes);
    }
    if(landed){o.player.cached_land_object_at_74=cached;add_displacement(o);o.result.outcome=accumulator;}
    else {airborne_scene_tail(o,false);o.result.outcome=accumulator;}
}
} // namespace
PlayerScenePositionResult position_player_in_scene(PlayerTaskState& task,PlayerMotionState* world,
    PlayerPhysicsState& player,PlayerParameters& parameters,PlayerSceneLandscape& scene,const PlayerScenePositionConstants& constants,
    PlayerScenePositionRuntime& runtime,PlayerCoordinates& coordinates,X87Context& fp){
    SceneOperation operation(task,world,player,parameters,scene,constants,runtime,coordinates,fp);
    try{initial_scene(operation,false);if(!early_scene_exit(operation,false))ordinary_scene_pass(operation);}
    catch(Stopped){}catch(const CoordinateError& error){operation.result.failure=ScenePositionFailure::coordinate_failed;operation.result.coordinate.failure=error.failure();}
    return operation.result;
}
PlayerScenePositionResult position_player_in_scene_alternate(PlayerTaskState& task,PlayerMotionState* world,
    PlayerPhysicsState& player,PlayerParameters& parameters,PlayerSceneLandscape& scene,const PlayerScenePositionConstants& constants,
    PlayerScenePositionRuntime& runtime,PlayerCoordinates& coordinates,X87Context& fp){
    SceneOperation operation(task,world,player,parameters,scene,constants,runtime,coordinates,fp);
    try{initial_scene(operation,true);if(!early_scene_exit(operation,true))alternate_scene_pass(operation);}
    catch(Stopped){}catch(const CoordinateError& error){operation.result.failure=ScenePositionFailure::coordinate_failed;operation.result.coordinate.failure=error.failure();}
    return operation.result;
}
} // namespace sadx
