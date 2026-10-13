#include "player/collision_normal.hpp"
#include <cstring>
namespace sadx {
namespace {
struct NormalFailure { CollisionNormalResult result; };
[[noreturn]] void failure(CollisionNormalFailure f,std::uint32_t address,std::uint32_t nested=0) {
    CollisionNormalResult r;r.failure=f;r.source_boundary=address;r.nested_source_boundary=nested;throw NormalFailure{r};
}
template<typename T>T required(const std::optional<T>& value,CollisionNormalFailure f,std::uint32_t address) {
    if(!value)failure(f,address);return *value;
}
PositionResourceRef ref(const std::optional<PositionResourceRef>& value,std::uint32_t address) {
    return required(value,CollisionNormalFailure::unavailable_reference,address);
}
template<typename T>const T& resource(const std::unordered_map<std::size_t,T>& table,PositionResourceRef id,std::uint32_t address) {
    if(!id.id)failure(CollisionNormalFailure::null_required_reference,address);
    const auto found=table.find(*id.id);if(found==table.end())failure(CollisionNormalFailure::unresolved_resource,address);return found->second;
}
const PositionCollisionInfo& info(const PlayerPositionCollisionScene& scene,PositionResourceRef table,std::int16_t index,std::uint32_t address) {
    const auto& records=resource(scene.info_tables,table,address).records;const auto it=records.find(index);
    if(it==records.end())failure(CollisionNormalFailure::unavailable_shape,address);return it->second;
}
CollisionTaskView lazy_task(PositionResourceRef id,const CollisionTaskLookup& lookup) {
    if(!id.id)return collision_task_view(nullptr);
    CollisionTaskView view;view.present=true;
    // The source's absolute-center branch never dereferences its task pointer.
    // Do not resolve an otherwise unused task owner before that branch.
    view.position=[id,&lookup]()->std::optional<NinjaVector>{
        if(!lookup)return {};auto resolved=lookup(id);
        if(!resolved||!resolved->present||!*resolved->present||!resolved->position)return {};return resolved->position();};
    view.angle=[id,&lookup](unsigned index)->std::optional<std::int32_t>{
        if(!lookup)return {};auto resolved=lookup(id);
        if(!resolved||!resolved->present||!*resolved->present||!resolved->angle)return {};return resolved->angle(index);};
    return view;
}
void shape_ok(CollisionShapeResult result,std::uint32_t address) {
    if(!result) {CollisionNormalResult r;r.failure=CollisionNormalFailure::shape_unavailable;
        r.source_boundary=address;r.nested_source_boundary=result.source_boundary;r.shape=result;throw NormalFailure{r};}
}
std::int32_t signed32(std::uint32_t bits){std::int32_t out;std::memcpy(&out,&bits,4);return out;}
void store(CollisionNormalOutput& out,NinjaVector v,std::uint8_t& stores) {
    out.x=v.x;stores|=1;out.y=v.y;stores|=2;out.z=v.z;stores|=4;
}
}
CollisionNormalResult collision_normal_query(const PlayerTaskState& task,const PlayerPositionCollisionScene& scene,
    const CollisionTaskLookup& lookup,const CoordinateAxisConstants& constants,PlayerCoordinates* coordinates,
    CollisionNormalOutput& output,X87Context& fp) {
    std::uint8_t stores=0;
    try {
        const auto& own=resource(scene.works,ref(scene.task_collision,0x41bc08),0x41bc0b);
        const auto hit_ref=ref(own.hit_work,0x41bc0b);
        CollisionNormalResult result;
        if(!hit_ref.id){result.contact=0;return result;}
        const auto& hit=resource(scene.works,hit_ref,0x41bc2b);
        const auto index=required(own.my_info_index,CollisionNormalFailure::unavailable_info_index,0x41bc28);
        const auto count=required(hit.info_count,CollisionNormalFailure::unavailable_info_count,0x41bc2b);
        if(static_cast<std::int32_t>(index)>static_cast<std::int32_t>(count)){result.contact=0;return result;}
        const auto hit_info_ref=ref(hit.info,0x41bc37);
        const auto own_info_ref=ref(own.info,0x41bc1a);
        if(!own_info_ref.id || (!hit_info_ref.id && index==0)){result.contact=0;return result;}
        const auto& own_info=info(scene,own_info_ref,0,0x41bc52);
        const auto& other_info=info(scene,hit_info_ref,index,0x41bc62);
        const auto own_a=required(own_info.a,CollisionNormalFailure::unavailable_shape_field,0x41bc52);
        const auto task_ref=ref(hit.owning_task,0x41bc55);
        const auto& other_task=resource(scene.tasks,task_ref,0x41bc5b);
        const auto task_view=lazy_task(ref(other_task.work,0x41bc5b),lookup);
        const auto other_a=required(other_info.a,CollisionNormalFailure::unavailable_shape_field,0x41bc62);
        const auto other_c=required(other_info.c,CollisionNormalFailure::unavailable_shape_field,0x41bc69);
        NinjaVector own_center;
        shape_ok(collision_shape_center(collision_task_view(&task),own_info,own_center,coordinates,fp),0x41bc77);
        const auto form=required(other_info.form,CollisionNormalFailure::unavailable_shape_field,0x41bc7c);
        if(form>7||form==5){result.contact=0;return result;}
        const auto zero=constants.zero;
        if(form==3) {
            own_center=required(task.position,CollisionNormalFailure::unavailable_task_position,0x41bd44);
            NinjaVector other_center;
            shape_ok(collision_shape_box_center(task_view,other_info,other_center,coordinates,fp),0x41bd60);
            const auto attr=required(other_info.attributes,CollisionNormalFailure::unavailable_shape_field,0x41bd65);
            std::uint32_t angle=static_cast<std::uint32_t>(required(other_info.angle_y,CollisionNormalFailure::unavailable_shape_field,0x41bd7b));
            if(!(attr&0x20)&&!(attr&0x8000)) {
                if(!task_view.angle)failure(CollisionNormalFailure::unavailable_task_yaw,0x41bd73);
                angle+=static_cast<std::uint32_t>(required(task_view.angle(1),CollisionNormalFailure::unavailable_task_yaw,0x41bd73));
            }
            const auto rotation=signed32(0U-angle);
            NinjaVector delta;
            shape_ok(collision_relative_yaw(own_center,other_center,rotation,delta,coordinates,fp),0x41bd8c);
            const auto a=fp.add_float_memory(fp.from_float(other_a),own_a);
            const auto weighted_z=fp.multiply_float_memory(a,delta.z);
            const auto stored_z=fp.to_float(weighted_z);
            const auto c=fp.add_float_memory(fp.from_float(other_c),own_a);
            const auto weighted_x=fp.multiply_float_memory(c,delta.x);
            const auto stored_x=fp.to_float(weighted_x); // FST retains ext80
            const auto sum=fp.add_float_memory(weighted_x,stored_z);
            const auto difference=fp.subtract_float_memory(fp.from_float(stored_z),stored_x);
            const auto difference_compare=fp.compare_float_memory(difference,zero);
            const auto sum_compare=fp.compare_float_memory(sum,zero);
            const bool difference_positive=difference_compare==X87Comparison::greater;
            const bool sum_positive=sum_compare==X87Comparison::greater;
            const std::uint32_t quadrant=difference_positive?(sum_positive?0U:0xc000U):(sum_positive?0x4000U:0x8000U);
            if(!coordinates)failure(CollisionNormalFailure::missing_matrix_owner,0x41be0e);
            (void)coordinates->push_unit_frame();
            const auto final_rotation=signed32(quadrant-static_cast<std::uint32_t>(rotation));
            if(final_rotation)coordinates->rotate_y(coordinates->current_matrix(),final_rotation,fp);
            NinjaVector normal;
            const auto transformed=coordinates->calculate_vector(coordinates->current_matrix(),{0,0,1},normal,fp);
            if(!transformed)failure(CollisionNormalFailure::coordinate_unavailable,0x41be2c,transformed.source_boundary);
            store(output,normal,stores);coordinates->end_frame();result.contact=1;result.output_stores=stores;return result;
        }
        NinjaVector other_center;
        // Forms2/6 invoke419030; the bytes and behavior match419270, which
        // supplies the remaining selected forms. No shape labels are guessed.
        const auto transformed_other=(form==2||form==6)
            ?collision_shape_center_419030(task_view,other_info,other_center,coordinates,fp)
            :collision_shape_center(task_view,other_info,other_center,coordinates,fp);
        shape_ok(transformed_other,form==2?0x41bd29:form==6?0x41bd3a:form==0||form==7?0x41bc9a:0x41be4b);
        const auto dx=fp.subtract_float_memory(fp.from_float(own_center.x),other_center.x);
        std::optional<X87Value> squared;
        if(form==0||form==7) {
            output.x=fp.to_float(dx);stores|=1;
            const auto dy=fp.subtract_float_memory(fp.from_float(own_center.y),other_center.y);
            output.y=fp.to_float(dy);stores|=2;
            const auto dz=fp.subtract_float_memory(fp.from_float(own_center.z),other_center.z);
            output.z=fp.to_float(dz);stores|=4;
            const auto yy=fp.multiply(dy,dy),zz=fp.multiply(dz,dz);
            const auto yz=fp.add(yy,zz);const auto xx=fp.multiply(dx,dx);
            squared=fp.add(yz,xx);
        } else {
            output.y=0;stores|=2;output.x=fp.to_float(dx);stores|=1;
            const auto dz=fp.subtract_float_memory(fp.from_float(own_center.z),other_center.z);
            output.z=fp.to_float(dz);stores|=4;
            const auto zz=fp.multiply(dz,dz),xx=fp.multiply(dx,dx);squared=fp.add(zz,xx);
        }
        if(fp.compare_float_memory(*squared,zero)==X87Comparison::equal)store(output,{0,0,1},stores);
        else {
            auto normal=*output.complete();
            try {(void)normalize_vector_extended(normal,constants,fp);}
            catch(const X87Error&){store(output,normal,stores);throw;}
            store(output,normal,stores);
        }
        result.contact=1;result.output_stores=stores;return result;
    }catch(const NormalFailure& failure){auto r=failure.result;r.output_stores=stores;return r;}
    catch(const CoordinateError&){CollisionNormalResult r;r.failure=CollisionNormalFailure::invalid_matrix_state;r.source_boundary=0x41be0e;r.output_stores=stores;return r;}
}
std::function<PositionNormalResult(PlayerTaskState&,PlayerPositionCollisionScene&,X87Context&)>
    bind_collision_normal(const CollisionTaskLookup& lookup,const CoordinateAxisConstants& constants,PlayerCoordinates* coordinates) {
    // The copied callable retains native owners selected by its captures; the
    // coordinate resource itself must outlive this binding, as other core views.
    return [lookup,constants,coordinates](PlayerTaskState& task,PlayerPositionCollisionScene& scene,X87Context& fp){
        CollisionNormalOutput output;
        const auto result=collision_normal_query(task,scene,lookup,constants,coordinates,output,fp);
        return PositionNormalResult{result?PlayerPositionFailure::none:PlayerPositionFailure::normal_owner_failed,
                                    output.complete(),result.source_boundary};};
}
} // namespace sadx
