#include "player/player_mesh_query.hpp"
#include "cpu/crt_integer.hpp"
#include "player/player_rotation.hpp"
#include <cstring>
#include <unordered_set>

namespace sadx {
namespace {
bool parity41(X87Comparison v){return v==X87Comparison::greater || v==X87Comparison::unordered;}
bool condition0(X87Comparison v){return v==X87Comparison::less || v==X87Comparison::unordered;}
std::int32_t signed_word(std::uint32_t u){std::int32_t s;std::memcpy(&s,&u,4);return s;}
}
PlayerMeshQueryResult query_mesh_object_contacts(PlayerContactWork& contact,PlayerSceneLandscape& scene,
    const SceneResourceReference& object,PlayerMeshQueryState& state,const PlayerMeshQueryConstants& c,
    const FloorAngleConstants& floor,const CoordinateAxisConstants& axis,PlayerMeshQueryRuntime& runtime,
    PlayerCoordinates& coordinates,X87Context& fp) {
    PlayerMeshQueryResult result;std::uint32_t matrix_site=0;
    const auto fail=[&](PlayerMeshQueryFailure why,std::uint32_t site){result.failure=why;result.source_boundary=site;return result;};
    const auto complete=[&](std::int32_t outcome){result.outcome=outcome;return result;};
    const auto load=[&](float f){return fp.from_float(f);};
    const auto compare=[&](float a,float b){return fp.compare_float_memory(load(a),b);};
    const auto add=[&](float a,float b){return fp.to_float(fp.add_float_memory(load(a),b));};
    const auto sub=[&](float a,float b){return fp.to_float(fp.subtract_float_memory(load(a),b));};
    const auto subtract=[&](const NinjaVector& a,const NinjaVector& b){return NinjaVector{sub(a.x,b.x),sub(a.y,b.y),sub(a.z,b.z)};};
    const auto sum=[&](const NinjaVector& a,const NinjaVector& b){return NinjaVector{add(a.x,b.x),add(a.y,b.y),add(a.z,b.z)};};
    const auto multiplied=[&](const NinjaVector& v,float factor){return NinjaVector{
        fp.to_float(fp.multiply_float_memory(load(v.x),factor)),fp.to_float(fp.multiply_float_memory(load(v.y),factor)),fp.to_float(fp.multiply_float_memory(load(v.z),factor))};};
    try {
        if(!contact.radius_at_0)return fail(PlayerMeshQueryFailure::unavailable_contact_field,0x45382e);
        const float radius=*contact.radius_at_0;
        if(!parity41(compare(radius,axis.zero)))return complete(0);
        state.contact_count=0;
        if(!contact.displacement_at_10)return fail(PlayerMeshQueryFailure::unavailable_contact_field,0x453854);
        const auto displacement=*contact.displacement_at_10;
        const auto length=fp.to_float(rotation_vector_length(displacement,fp));
        const auto half=fp.to_float(fp.multiply_float_memory(load(radius),c.half_radius));
        if(!scene.mesh_groups)return fail(PlayerMeshQueryFailure::unavailable_group_selector,0x45387e);
        auto group_ref=*scene.mesh_groups;const SceneMeshBoundGroup* group=nullptr;std::unordered_set<std::size_t> seen;
        while(group_ref.id){
            if(!seen.insert(*group_ref.id).second)return fail(PlayerMeshQueryFailure::cyclic_groups,0x453890);
            const auto found=scene.groups.find(*group_ref.id);if(found==scene.groups.end())return fail(PlayerMeshQueryFailure::unresolved_group,0x453896);
            if(!found->second.object)return fail(PlayerMeshQueryFailure::unavailable_group_field,0x453899);
            if(found->second.object->id==object.id){group=&found->second;break;}
            if(!found->second.next)return fail(PlayerMeshQueryFailure::unavailable_group_field,0x45388b);group_ref=*found->second.next;
        }
        if(!group)return complete(0);
        const auto count=signed_word(group->selected_count);
        if(count<=0)return complete(0);
        if(!group->selected_begin)return fail(PlayerMeshQueryFailure::unavailable_group_field,0x4538c5);
        if(*group->selected_begin>scene.selected_nodes.size() || static_cast<std::size_t>(count)>scene.selected_nodes.size()-*group->selected_begin)
            return fail(PlayerMeshQueryFailure::unavailable_selected_node,0x4538ff);
        for(std::size_t index=0;index<static_cast<std::size_t>(count);++index){
            const auto id=scene.selected_nodes[*group->selected_begin+index];if(!id)return fail(PlayerMeshQueryFailure::unavailable_selected_node,0x4538ff);
            result.node=*id;const auto found=scene.nodes.find(*id);if(found==scene.nodes.end())return fail(PlayerMeshQueryFailure::unresolved_node,0x453901);
            const auto& node=found->second;if(!node.triangle_vertices)return fail(PlayerMeshQueryFailure::unavailable_triangle,0x453901);
            const auto origin=(*node.triangle_vertices)[0];
            std::array<NinjaVector,3> triangle{{{0,0,0},subtract((*node.triangle_vertices)[1],origin),subtract((*node.triangle_vertices)[2],origin)}};
            if(!contact.center_at_4)return fail(PlayerMeshQueryFailure::unavailable_contact_field,0x453b24);
            auto center=subtract(*contact.center_at_4,origin);auto velocity=displacement;
            const auto centroid_component=[&](float a,float b,float d){return fp.to_float(fp.divide_float_memory(fp.add_float_memory(fp.add_float_memory(load(a),b),d),c.three));};
            const NinjaVector centroid{centroid_component(triangle[0].x,triangle[1].x,triangle[2].x),centroid_component(triangle[0].y,triangle[1].y,triangle[2].y),centroid_component(triangle[0].z,triangle[1].z,triangle[2].z)};
            for(auto& vertex:triangle)vertex=subtract(vertex,centroid);
            matrix_site=0x453c12;coordinates.push_unit_frame();
            matrix_site=0x453c28;coordinates.scale_matrix(coordinates.current_matrix(),c.triangle_expansion,c.triangle_expansion,c.triangle_expansion,fp);
            for(std::size_t n=0;n<3;++n){matrix_site=n==0?0x453c36:n==1?0x453c44:0x453c52;coordinates.calculate_point(coordinates.current_matrix(),triangle[n],triangle[n],fp);}
            for(auto& vertex:triangle)vertex=sum(vertex,centroid);
            matrix_site=0x453cc8;coordinates.end_frame();
            matrix_site=0x453cd2;coordinates.push_unit_frame();
            NinjaVector normal;mesh_triangle_normal(triangle[0],triangle[1],triangle[2],normal,fp);
            result.normal_normalization=normalize_mesh_vector_up(normal,axis,runtime.inverse_math,fp);
            if(!*result.normal_normalization){result.nested_source_boundary=result.normal_normalization->sqrt->source_boundary;return fail(PlayerMeshQueryFailure::normalization_unavailable,0x453cf3);}
            result.normal_angles=floor_normal_angles(normal,floor,runtime.inverse_math,fp);
            if(result.normal_angles->failure!=CrtAsinFailure::none || !result.normal_angles->angles){result.nested_source_boundary=result.normal_angles->source_boundary;return fail(PlayerMeshQueryFailure::normal_angles_unavailable,0x453d15);}
            const auto inclination=0U-static_cast<std::uint32_t>(result.normal_angles->angles->inclination);
            const auto roll=0U-static_cast<std::uint32_t>(result.normal_angles->angles->roll);
            if(inclination){matrix_site=0x453d36;coordinates.rotate_x(coordinates.current_matrix(),signed_word(inclination),fp);}
            if(roll){matrix_site=0x453d54;coordinates.rotate_z(coordinates.current_matrix(),signed_word(roll),fp);}
            matrix_site=0x453d62;coordinates.calculate_point(coordinates.current_matrix(),triangle[1],triangle[1],fp);
            matrix_site=0x453d70;coordinates.calculate_point(coordinates.current_matrix(),triangle[2],triangle[2],fp);
            matrix_site=0x453d7e;coordinates.calculate_point(coordinates.current_matrix(),center,center,fp);
            matrix_site=0x453d8c;coordinates.calculate_point(coordinates.current_matrix(),velocity,velocity,fp);
            if(!node.raw_at_3c)return fail(PlayerMeshQueryFailure::unavailable_material_flags,0x453d9e);
            const bool double_sided=(*node.raw_at_3c&0x800000U)!=0;
            float signed_radius=radius;
            if(double_sided && compare(center.y,axis.zero)==X87Comparison::less)
                signed_radius=fp.to_float(fp.multiply_float_memory(load(signed_radius),c.negative_one));
            auto step=velocity;result.direction_normalization=normalize_mesh_vector_zero(step,axis,runtime.inverse_math,fp);
            if(!*result.direction_normalization){result.nested_source_boundary=result.direction_normalization->sqrt->source_boundary;return fail(PlayerMeshQueryFailure::normalization_unavailable,0x453df8);}
            step=multiplied(step,half);NinjaVector sample=center;float traveled=0;bool accepted=false;std::size_t attempts=0;
            while(true){
                if(runtime.sample_budget && attempts++>=*runtime.sample_budget)return fail(PlayerMeshQueryFailure::sample_budget_exhausted,0x453e99);
                const auto progress_compare=compare(traveled,length);
                const bool final=!condition0(progress_compare);
                if(final)sample=sum(center,velocity);
                result.sample=sample;result.traveled=traveled;
                const auto argument=fp.to_double(load(sample.y));result.fabs=crt_fabs(argument,runtime.absolute_math,fp);
                if(!*result.fabs || !result.fabs->value){result.nested_source_boundary=result.fabs->source_boundary;return fail(PlayerMeshQueryFailure::fabs_unavailable,0x453f08);}
                (void)fp.to_float(*result.fabs->value); // sourceFST32 before retained compare
                if(!parity41(fp.compare_float_memory(*result.fabs->value,radius))){
                    std::array<float,3> edge{};
                    for(unsigned n=0;n<3;++n){
                        const auto& a=triangle[n];const auto& b=triangle[(n+1)%3];
                        const auto dz=fp.subtract_float_memory(load(b.z),a.z);
                        const auto px=fp.subtract_float_memory(load(sample.x),a.x);
                        const auto first=fp.multiply(dz,px);
                        const auto dx=fp.subtract_float_memory(load(b.x),a.x);
                        const auto pz=fp.subtract_float_memory(load(sample.z),a.z);
                        edge[n]=fp.to_float(fp.subtract(first,fp.multiply(dx,pz)));
                    }
                    const auto e01=fp.compare_float_memory(fp.multiply_float_memory(load(edge[0]),edge[1]),axis.zero);
                    if(!condition0(e01)){
                        const auto e12=fp.compare_float_memory(fp.multiply_float_memory(load(edge[1]),edge[2]),axis.zero);
                        if(!condition0(e12)){
                            const auto e20=fp.compare_float_memory(fp.multiply_float_memory(load(edge[2]),edge[0]),axis.zero);
                            if(!condition0(e20))accepted=true;
                        }
                    }
                }
                if(accepted || final)break;
                const auto previous=sample;const float previous_traveled=traveled;
                sample=sum(sample,step);traveled=add(traveled,half);
                if(std::memcmp(&previous,&sample,sizeof(sample))==0 && std::memcmp(&previous_traveled,&traveled,4)==0)
                    return fail(PlayerMeshQueryFailure::sampling_stagnated,0x453e99);
            }
            if(accepted){
                const auto end_y=add(center.y,velocity.y); // source454024 precedes orientation/CRT math
                NinjaVector oriented=normal;
                if(double_sided && condition0(compare(center.y,axis.zero))){
                    oriented.x=fp.to_float(load(normal.x).negated());oriented.y=fp.to_float(load(normal.y).negated());oriented.z=fp.to_float(load(normal.z).negated());
                }
                // Source stores Y binary64 first, then X; atan2formal(y,x).
                const auto argument_x=fp.to_double(load(oriented.y));const auto argument_y=fp.to_double(load(oriented.x));
                result.atan2=crt_atan2_binary64(argument_y,argument_x,runtime.planar_angle,fp);
                if(result.atan2->failure!=CrtAtan2Failure::none || !result.atan2->value){result.nested_source_boundary=result.atan2->source_boundary;return fail(PlayerMeshQueryFailure::atan2_unavailable,0x4540d1);}
                const auto z_scaled=fp.divide_double_memory(fp.multiply_double_memory(*result.atan2->value,c.angle_units),c.angle_divisor);
                const auto angle_z=signed_word(0U-crt_ftol_low32(z_scaled,fp));
                const auto argument_z=fp.to_double(load(oriented.z));result.asin=crt_asin_binary64(argument_z,runtime.inverse_math,fp);
                if(result.asin->failure!=CrtAsinFailure::none || !result.asin->value){result.nested_source_boundary=result.asin->source_boundary;return fail(PlayerMeshQueryFailure::asin_unavailable,0x4540fe);}
                const auto x_scaled=fp.divide_double_memory(fp.multiply_double_memory(*result.asin->value,c.angle_units),c.angle_divisor);
                const auto angle_x=signed_word(crt_ftol_low32(x_scaled,fp));
                matrix_site=0x45411f;(void)invert_mesh_matrix(coordinates.current_matrix(),axis,fp);
                NinjaVector point{center.x,0,center.z},transformed;
                matrix_site=0x45414f;coordinates.calculate_point(coordinates.current_matrix(),point,transformed,fp);const auto on_plane=sum(transformed,origin);
                point.y=signed_radius;matrix_site=0x4541a8;coordinates.calculate_point(coordinates.current_matrix(),point,transformed,fp);const auto on_radius=sum(transformed,origin);
                // Source performs an unused sign test/store before resetting Y.
                if(double_sided){const auto test=fp.compare_float_memory(fp.multiply_float_memory(load(center.y),velocity.y),axis.zero);if(test==X87Comparison::less)(void)fp.to_float(load(velocity.y).negated());}
                else{const auto test=fp.compare_float_memory(fp.multiply_float_memory(load(signed_radius),velocity.y),axis.zero);if(test==X87Comparison::less)(void)fp.to_float(load(velocity.y).negated());}
                point={velocity.x,0,velocity.z};matrix_site=0x454273;coordinates.calculate_point(coordinates.current_matrix(),point,transformed,fp);const auto horizontal=transformed;
                point={0,velocity.y,0};matrix_site=0x4542c8;coordinates.calculate_point(coordinates.current_matrix(),point,transformed,fp);
                auto& record=state.records[*state.contact_count];record.node=*id;record.node_snapshot=node;
                record.payload=MeshContactPayload{angle_x,angle_z,normal,oriented,on_plane,on_radius,horizontal,transformed,center.y,end_y};
                ++*state.contact_count;
                if(*state.contact_count==16)break; // sourcecap skips following PopMatrix
            }
            matrix_site=0x454341;coordinates.end_frame();
        }
        if(*state.contact_count<1)return complete(0);
        std::size_t selected=0;float minimum=state.records[0].payload->plane_end_y;
        for(std::size_t n=1;n<*state.contact_count;++n){const auto value=state.records[n].payload->plane_end_y;
            if(compare(value,minimum)==X87Comparison::less){minimum=value;selected=n;}}
        for(std::size_t n=0;n<*state.contact_count;++n)state.responses[n]=state.records[n].payload;
        contact.contact_count_at_28=static_cast<std::uint16_t>(*state.contact_count);contact.selected_contact_at_2a=static_cast<std::uint16_t>(selected);
        const auto& chosen=*state.records[selected].payload;contact.angle_x_at_34=chosen.angle_x;contact.angle_z_at_38=chosen.angle_z;
        contact.vector_at_3c=chosen.normal;contact.vector_at_48=chosen.oriented_normal;contact.vector_at_6c=chosen.vector_at_38;contact.vector_at_78=chosen.vector_at_44;
        if(*state.contact_count==1){contact.vector_at_54=chosen.point_at_20;contact.vector_at_60=chosen.point_at_2c;}
        else{
            contact.vector_at_54=state.records[0].payload->point_at_20;contact.vector_at_60=state.records[0].payload->point_at_2c;
            const auto add_into=[&](NinjaVector& dst,const NinjaVector& src){dst.x=add(dst.x,src.x);dst.y=add(dst.y,src.y);dst.z=add(dst.z,src.z);};
            for(std::size_t n=1;n<*state.contact_count;++n){add_into(*contact.vector_at_54,state.records[n].payload->point_at_20);add_into(*contact.vector_at_60,state.records[n].payload->point_at_2c);}
            const auto inverse=fp.to_float(fp.reverse_divide_float_memory(fp.from_int64(static_cast<std::int32_t>(*state.contact_count)),axis.one));
            const auto multiply_into=[&](NinjaVector& dst){dst.x=fp.to_float(fp.multiply_float_memory(load(inverse),dst.x));dst.y=fp.to_float(fp.multiply_float_memory(load(inverse),dst.y));dst.z=fp.to_float(fp.multiply_float_memory(load(inverse),dst.z));};
            multiply_into(*contact.vector_at_54);multiply_into(*contact.vector_at_60);
        }
        return complete(1);
    }catch(const CoordinateError& error){result.coordinate_failure=error.failure();return fail(PlayerMeshQueryFailure::invalid_matrix_owner,matrix_site);}
}
} // namespace sadx
