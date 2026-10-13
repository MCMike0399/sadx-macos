#include "player/collision_shapes.hpp"
namespace sadx {
CollisionTaskView collision_task_view(const PlayerTaskState* task) {
    CollisionTaskView view;view.present=task!=nullptr;
    if(task) {
        view.position=[task]{return task->position;};
        view.angle=[task](unsigned i)->std::optional<std::int32_t>{
            if(i>=3)return {};return task->orientation.angles[i];};
    }
    return view;
}
namespace {
struct ShapeFailure { CollisionShapeResult result; };
[[noreturn]] void failure(CollisionShapeFailure f,std::uint32_t address,std::uint32_t nested=0){throw ShapeFailure{{f,address,nested}};}
void task_present(const CollisionTaskView& task,std::uint32_t address) {
    if(!task.present)failure(CollisionShapeFailure::unavailable_task_presence,address);
    if(!*task.present)failure(CollisionShapeFailure::null_required_task,address);
}
NinjaVector position(const CollisionTaskView& task,std::uint32_t address) {
    task_present(task,address);if(!task.position)failure(CollisionShapeFailure::missing_pose_owner,address);
    const auto value=task.position();if(!value)failure(CollisionShapeFailure::unavailable_task_position,address);return *value;
}
std::int32_t angle(const CollisionTaskView& task,unsigned index,std::uint32_t address) {
    task_present(task,address);if(!task.angle)failure(CollisionShapeFailure::missing_pose_owner,address);
    const auto value=task.angle(index);if(!value)failure(CollisionShapeFailure::unavailable_task_angle,address);return *value;
}
PlayerCoordinates& matrix(PlayerCoordinates* value,std::uint32_t address) {
    if(!value)failure(CollisionShapeFailure::missing_matrix_owner,address);return *value;
}
std::uint32_t center_address(std::uint32_t address,bool entry030) {
    if(!entry030)return address;
    if(address==0x419276||address==0x41927d)return address-0x240;
    return address-0x23a;
}
void rotate_task(const CollisionTaskView& task,std::uint32_t attributes,PlayerCoordinates& c,
                 bool box,bool entry030,X87Context& fp) {
    // The bytecode reads each task angle and conditionally executes that axis;
    // it does not require unused later fields before earlier source mutations.
    const unsigned yzx[]={1,2,0},zxy[]={2,0,1};const bool alternate=(attributes&0x200)!=0;
    const auto order=alternate?yzx:zxy;
    const std::uint32_t regular[]={0x4192f3,0x419304,0x419315},changed[]={0x4192be,0x4192cf,0x4192e0};
    const std::uint32_t box_regular[]={0x41941a,0x41942b,0x41943c},box_changed[]={0x4193e5,0x4193f6,0x419407};
    const auto addresses=box?(alternate?box_changed:box_regular):(alternate?changed:regular);
    for(unsigned n=0;n<3;++n) {
        const auto value=angle(task,order[n],box?addresses[n]:center_address(addresses[n],entry030));
        if(!value)continue;
        if(order[n]==0)c.rotate_x(c.current_matrix(),value,fp);
        else if(order[n]==1)c.rotate_y(c.current_matrix(),value,fp);
        else c.rotate_z(c.current_matrix(),value,fp);
    }
}
CollisionShapeResult center(const CollisionTaskView& task,const PositionCollisionInfo& shape,
    NinjaVector& output,PlayerCoordinates* owner,X87Context& fp,bool box,bool entry030) {
    try {
        if(!shape.attributes)failure(CollisionShapeFailure::unavailable_attributes,box?0x4193a5:center_address(0x419276,entry030));
        const auto attr=*shape.attributes;
        if(attr&0x20) {
            if(!shape.center)failure(CollisionShapeFailure::unavailable_center,box?0x4193ab:center_address(0x41927d,entry030));
            output=*shape.center;return {};
        }
        if(!box && (attr&0x8000)) {
            const auto p=position(task,center_address(0x419343,entry030));
            if(!shape.center)failure(CollisionShapeFailure::unavailable_center,center_address(0x419346,entry030));
            // Raw actor components load before Z/Y/X arithmetic and stores.
            const auto px=fp.from_float(p.x),py=fp.from_float(p.y),pz=fp.from_float(p.z);
            output.z=fp.to_float(fp.add(fp.from_float(shape.center->z),pz));
            output.y=fp.to_float(fp.add(fp.from_float(shape.center->y),py));
            output.x=fp.to_float(fp.add(fp.from_float(shape.center->x),px));return {};
        }
        auto& c=matrix(owner,box?0x4193c8:center_address(0x41929e,entry030));
        if(box)(void)c.push_unit_frame();else (void)c.begin_unit_frame();
        c.translate_matrix(c.current_matrix(),position(task,box?0x4193d1:center_address(0x4192ae,entry030)),fp);
        if(!(box&&(attr&0x8000)))rotate_task(task,attr,c,box,entry030,fp);
        if(!shape.center)failure(CollisionShapeFailure::unavailable_center,box?0x419452:center_address(0x41932b,entry030));
        c.calculate_point(c.current_matrix(),*shape.center,output,fp);
        c.end_ex_frame();return {};
    }catch(const ShapeFailure& e){return e.result;}
    catch(const CoordinateError&){return {CollisionShapeFailure::invalid_matrix_state,box?0x4193c8U:center_address(0x41929e,entry030),0};}
}
}
CollisionShapeResult collision_shape_center(const CollisionTaskView& task,const PositionCollisionInfo& shape,
    NinjaVector& output,PlayerCoordinates* c,X87Context& fp){return center(task,shape,output,c,fp,false,false);}
CollisionShapeResult collision_shape_center_419030(const CollisionTaskView& task,const PositionCollisionInfo& shape,
    NinjaVector& output,PlayerCoordinates* c,X87Context& fp){return center(task,shape,output,c,fp,false,true);}
CollisionShapeResult collision_shape_box_center(const CollisionTaskView& task,const PositionCollisionInfo& shape,
    NinjaVector& output,PlayerCoordinates* c,X87Context& fp){return center(task,shape,output,c,fp,true,false);}
CollisionShapeResult collision_relative_yaw(const NinjaVector& first,const NinjaVector& second,
    std::int32_t yaw,NinjaVector& output,PlayerCoordinates* owner,X87Context& fp) {
    try {
        NinjaVector temporary;
        temporary.x=fp.to_float(fp.subtract_float_memory(fp.from_float(first.x),second.x));
        temporary.y=fp.to_float(fp.subtract_float_memory(fp.from_float(first.y),second.y));
        temporary.z=fp.to_float(fp.subtract_float_memory(fp.from_float(first.z),second.z));
        auto& c=matrix(owner,0x419924);(void)c.push_unit_frame();
        if(yaw)c.rotate_y(c.current_matrix(),yaw,fp);
        NinjaVector transformed;
        const auto result=c.calculate_vector(c.current_matrix(),temporary,transformed,fp);
        if(!result)failure(CollisionShapeFailure::coordinate_unavailable,0x419942,result.source_boundary);
        c.end_frame();output=transformed;return {};
    }catch(const ShapeFailure& e){return e.result;}
    catch(const CoordinateError&){return {CollisionShapeFailure::invalid_matrix_state,0x419924,0};}
}
} // namespace sadx
