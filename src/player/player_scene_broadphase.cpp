#include "player/player_scene_broadphase.hpp"
#include "player/player_rotation.hpp"
#include <unordered_set>

namespace sadx {
namespace {
SceneQueryResult failure(SceneQueryFailure why,std::uint32_t site){return {why,site,0,{},{}};}
bool parity41(X87Comparison v){return v==X87Comparison::greater || v==X87Comparison::unordered;}
template<class T>const T* resolve(const std::unordered_map<std::size_t,T>& map,const SceneResourceReference& ref){
    if(!ref.id)return nullptr;const auto it=map.find(*ref.id);return it==map.end()?nullptr:&it->second;
}
}
SceneQueryResult collect_scene_land_candidates(PlayerSceneLandscape& scene,const NinjaVector& center,
    float radius,const SceneBroadphaseConstants& constants,PlayerCoordinates& coordinates,X87Context& fp) {
    std::uint32_t matrix_site=0x43ace6;
    try {
    scene.candidate_count=0;coordinates.push_current_frame();
    if(!scene.dynamic_count)return failure(SceneQueryFailure::unavailable_field,0x43aceb);
    if(*scene.dynamic_count<0 || static_cast<std::size_t>(*scene.dynamic_count)>scene.dynamic_land_entries.size())
        return failure(SceneQueryFailure::unavailable_registry_storage,0x43acf3);
    const auto load=[&](float v){return fp.from_float(v);};
    const auto insert=[&](const SceneLandEntry& entry){scene.candidates[scene.candidate_count]=entry;++scene.candidate_count;};
    for(std::size_t index=static_cast<std::size_t>(*scene.dynamic_count);index>0;--index) {
        const auto& entry=scene.dynamic_land_entries[index-1];
        if(!entry.attributes || !entry.object)return failure(SceneQueryFailure::unavailable_field,0x43ad10);
        if(!entry.object->id)return failure(SceneQueryFailure::null_required_reference,0x43ad1b);
        const auto object=resolve(scene.objects,*entry.object);
        if(!object)return failure(SceneQueryFailure::unresolved_resource,0x43ad1b);
        if(!object->model || !object->position)return failure(SceneQueryFailure::unavailable_field,0x43ad1b);
        if(!object->model->id)return failure(SceneQueryFailure::null_required_reference,0x43ad1e);
        const auto model=resolve(scene.models,*object->model);
        if(!model)return failure(SceneQueryFailure::unresolved_resource,0x43ad1e);
        if(!model->center || !model->radius)return failure(SceneQueryFailure::unavailable_field,0x43ad1e);
        auto model_center=*model->center;
        if(*entry.attributes&0x10000000U) {
            if(!object->angles)return failure(SceneQueryFailure::unavailable_field,0x43ad3e);
            // UnitMatrix resets the CURRENT pushed frame only for this path.
            // begin_unit_frame would push again and change source ownership.
            matrix_site=0x43ad39;coordinates.set_current_unit_matrix();
            if((*object->angles)[2]){matrix_site=0x43ad4a;coordinates.rotate_z(coordinates.current_matrix(),(*object->angles)[2],fp);}
            if((*object->angles)[0]){matrix_site=0x43ad5b;coordinates.rotate_x(coordinates.current_matrix(),(*object->angles)[0],fp);}
            if((*object->angles)[1]){matrix_site=0x43ad6c;coordinates.rotate_y(coordinates.current_matrix(),(*object->angles)[1],fp);}
            matrix_site=0x43ad7e;
            coordinates.calculate_point(coordinates.current_matrix(),*model->center,model_center,fp);
        }
        const auto x=fp.reverse_subtract_float_memory(fp.add_float_memory(load(model_center.x),object->position->x),center.x);
        const auto y=fp.reverse_subtract_float_memory(fp.add_float_memory(load(model_center.y),object->position->y),center.y);
        const auto z=fp.reverse_subtract_float_memory(fp.add_float_memory(load(model_center.z),object->position->z),center.z);
        const auto bound=fp.add_float_memory(fp.add_float_memory(load(radius),*model->radius),constants.land_padding);
        const auto sum=fp.add(fp.add(fp.multiply(z,z),fp.multiply(y,y)),fp.multiply(x,x));
        if(fp.compare(fp.multiply(bound,bound),sum)==X87Comparison::greater) {
            if(!entry.task)return failure(SceneQueryFailure::unavailable_field,0x43addb);
            insert(entry);if(scene.candidate_count==1024){matrix_site=0x43aedc;coordinates.end_frame();return {};}
        }
    }
    matrix_site=0x43ae15;coordinates.end_frame();
    if(!scene.static_enabled)return failure(SceneQueryFailure::unavailable_field,0x43ae1a);
    if(*scene.static_enabled!=1)return {};
    if(!scene.landtable)return failure(SceneQueryFailure::unavailable_field,0x43ae27);
    if(!scene.landtable->id)return {};
    if(!scene.static_count)return failure(SceneQueryFailure::unavailable_field,0x43ae34);
    if(*scene.static_count<=0)return {};
    if(static_cast<std::size_t>(*scene.static_count)>scene.static_entries.size())
        return failure(SceneQueryFailure::unavailable_registry_storage,0x43ae50);
    for(std::size_t i=0;i<static_cast<std::size_t>(*scene.static_count);++i) {
        const auto& entry=scene.static_entries[i];
        if(!entry.attributes)return failure(SceneQueryFailure::unavailable_field,0x43ae50);
        if(!(*entry.attributes&0x400003U))continue;
        if(!entry.center)return failure(SceneQueryFailure::unavailable_field,0x43ae59);
        const auto x=fp.subtract_float_memory(load(entry.center->x),center.x);
        const auto y=fp.subtract_float_memory(load(entry.center->y),center.y);
        const auto z=fp.subtract_float_memory(load(entry.center->z),center.z);
        const auto squared=fp.to_float(fp.add(fp.add(fp.multiply(z,z),fp.multiply(y,y)),fp.multiply(x,x)));
        const auto distance=fp.subtract_float_memory(signed_sqrt_extended(squared,fp),radius);
        if(!entry.radius)return failure(SceneQueryFailure::unavailable_field,0x43ae90);
        if(fp.compare(distance,load(*entry.radius))==X87Comparison::less) {
            if(!entry.object)return failure(SceneQueryFailure::unavailable_field,0x43aea1);
            insert(SceneLandEntry{entry.attributes,entry.object,SceneResourceReference{}});
            if(scene.candidate_count==1024)return {};
        }
    }
    return {};
    }catch(const CoordinateError& error){auto result=failure(SceneQueryFailure::invalid_matrix_owner,matrix_site);result.coordinate_failure=error.failure();return result;}
}

SceneQueryResult query_player_scene_broadphase(PlayerSceneLandscape& scene,const PlayerContactWork& contact,
    const SceneBroadphaseConstants& c,PlayerCoordinates& coordinates,X87Context& fp) {
    scene.selected_count=0;
    if(!contact.displacement_at_10 || !contact.radius_at_0)return failure(SceneQueryFailure::unavailable_field,0x450c2d);
    const auto extended=fp.add_float_memory(rotation_vector_length(*contact.displacement_at_10,fp),*contact.radius_at_0);
    const auto extent=fp.to_float(extended);
    const auto extent_squared=fp.to_float(fp.multiply_float_memory(extended,extent));
    if(!contact.center_at_4)return failure(SceneQueryFailure::unavailable_field,0x450c49);
    const auto& center=*contact.center_at_4;
    auto collected=collect_scene_land_candidates(scene,center,*contact.radius_at_0,c,coordinates,fp);
    collected.extent=extent;collected.extent_squared=extent_squared;if(!collected)return collected;
    const auto fail=[&](SceneQueryFailure why,std::uint32_t site){auto r=failure(why,site);r.extent=extent;r.extent_squared=extent_squared;return r;};
    if(!scene.mesh_groups)return fail(SceneQueryFailure::unavailable_field,0x450c63);
    auto group_ref=*scene.mesh_groups;std::unordered_set<std::size_t> groups_seen;
    const auto load=[&](float v){return fp.from_float(v);};
    const auto compare=[&](float a,float b){return fp.compare_float_memory(load(a),b);};
    while(group_ref.id) {
        if(!groups_seen.insert(*group_ref.id).second)return fail(SceneQueryFailure::cyclic_resource_links,0x450c75);
        const auto found=scene.groups.find(*group_ref.id);if(found==scene.groups.end())return fail(SceneQueryFailure::unresolved_resource,0x450c7f);
        auto& group=found->second;group.selected_begin.reset();group.selected_count=0;
        if(!group.object)return fail(SceneQueryFailure::unavailable_field,0x450cce);
        bool selected=false;
        for(std::size_t i=0;i<static_cast<std::size_t>(scene.candidate_count);++i) {
            const auto& candidate=scene.candidates[i];
            if(!candidate || !candidate->object)return fail(SceneQueryFailure::unavailable_registry_storage,0x450ccb);
            if(candidate->object->id==group.object->id){selected=true;break;}
        }
        if(selected) {
            if(!group.head || !group.tail || !group.z_padding)return fail(SceneQueryFailure::unavailable_field,0x450ce9);
            auto head=*group.head;const auto tail=*group.tail;
            const auto low=fp.to_float(fp.subtract_float_memory(fp.subtract_float_memory(load(center.z),*group.z_padding),extent));
            const auto high=fp.to_float(fp.add_float_memory(fp.add_float_memory(load(center.z),*group.z_padding),extent));
            const auto node=[&](const SceneResourceReference& ref){return resolve(scene.nodes,ref);};
            if(head.id && tail.id) {
                const auto first=node(head),last=node(tail);if(!first || !last)return fail(SceneQueryFailure::unresolved_resource,0x450d2b);
                if(!first->center || !last->center)return fail(SceneQueryFailure::unavailable_field,0x450d2e);
                if(compare(first->center->z,low)==X87Comparison::less && compare(high,last->center->z)==X87Comparison::less) {
                    std::unordered_set<std::size_t> seen;
                    while(head.id) {
                        if(!seen.insert(*head.id).second)return fail(SceneQueryFailure::cyclic_resource_links,0x450d4b);
                        const auto value=node(head);if(!value)return fail(SceneQueryFailure::unresolved_resource,0x450d4b);
                        if(!value->center)return fail(SceneQueryFailure::unavailable_field,0x450d4e);
                        if(parity41(compare(value->center->z,low)))break;
                        if(!value->next)return fail(SceneQueryFailure::unavailable_field,0x450d5e);head=*value->next;
                    }
                }
            }
            bool forward=false;
            if(head.id) {
                const auto value=node(head);if(!value)return fail(SceneQueryFailure::unresolved_resource,0x450d77);
                if(!value->center)return fail(SceneQueryFailure::unavailable_field,0x450d7a);
                forward=compare(value->center->z,low)==X87Comparison::greater;
            }
            auto ref=forward?head:tail;std::unordered_set<std::size_t> seen;
            while(ref.id) {
                if(!seen.insert(*ref.id).second)return fail(SceneQueryFailure::cyclic_resource_links,forward?0x450d95:0x450ee3);
                const auto value=node(ref);if(!value)return fail(SceneQueryFailure::unresolved_resource,forward?0x450d9f:0x450eed);
                if(!value->center)return fail(SceneQueryFailure::unavailable_field,forward?0x450da2:0x450ef0);
                const auto z=value->center->z;
                if(forward ? compare(z,high)!=X87Comparison::less : compare(z,low)!=X87Comparison::greater)break;
                if(!value->radius_squared)return fail(SceneQueryFailure::unavailable_field,forward?0x450db9:0x450f07);
                const auto limit=fp.to_float(fp.multiply_float_memory(fp.add_float_memory(load(extent_squared),*value->radius_squared),c.node_distance_scale));
                const auto dx=fp.to_float(fp.subtract_float_memory(load(value->center->x),center.x));
                auto square=fp.to_float(fp.multiply_float_memory(load(dx),dx));
                bool accept=compare(square,limit)!=X87Comparison::greater;
                if(accept) {
                    const auto dy=fp.to_float(fp.subtract_float_memory(load(value->center->y),center.y));
                    square=fp.to_float(fp.add_float_memory(fp.multiply_float_memory(load(dy),dy),square));
                    accept=compare(square,limit)!=X87Comparison::greater;
                }
                if(accept) {
                    const auto dz=fp.subtract_float_memory(load(z),center.z);const auto spilled=fp.to_float(dz);
                    square=fp.to_float(fp.add_float_memory(fp.multiply_float_memory(dz,spilled),square));
                    accept=compare(square,limit)!=X87Comparison::greater;
                }
                if(accept) {
                    if(!group.selected_begin)group.selected_begin=scene.selected_count;
                    ++group.selected_count;scene.selected_nodes[scene.selected_count++]=*ref.id;
                    if(scene.selected_count==128)return {SceneQueryFailure::none,0,0,extent,extent_squared};
                }
                const auto& next=forward?value->next:value->previous;
                if(!next)return fail(SceneQueryFailure::unavailable_field,forward?0x450d90:0x450edd);ref=*next;
            }
        }
        if(!group.next)return fail(SceneQueryFailure::unavailable_field,0x450c70);group_ref=*group.next;
    }
    return {SceneQueryFailure::none,0,0,extent,extent_squared};
}
} // namespace sadx
