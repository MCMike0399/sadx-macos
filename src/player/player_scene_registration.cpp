#include "player/player_scene_registration.hpp"
#include <cstdio>

namespace sadx {
SceneQueryResult register_player_scene_landscape(PlayerSceneLandscape& scene,
    const DecodedSceneLandTable& table,const SceneRegistrationRuntime& runtime,
    const SceneRegistrationConstants& constants,PlayerCoordinates& coordinates,X87Context& fp) {
    std::uint32_t matrix_site=0x43afa3;
    try {
    const auto fail=[](SceneQueryFailure why,std::uint32_t site){return SceneQueryResult{why,site,0,{},{}};};
    if(!runtime.camera)return fail(SceneQueryFailure::unavailable_field,0x43aef0);
    if(!runtime.camera->task)return {};
    if(!runtime.camera->task->position)return fail(SceneQueryFailure::unavailable_field,0x43af0b);
    std::array<const NinjaVector*,2> views{{&*runtime.camera->task->position,nullptr}};std::size_t view_count=1;
    if(!runtime.first_player)return fail(SceneQueryFailure::unavailable_field,0x43af1b);
    if(runtime.first_player->task){
        if(!runtime.first_player->task->position)return fail(SceneQueryFailure::unavailable_field,0x43af33);
        views[0]=&*runtime.first_player->task->position;
    }
    if(!runtime.second_player)return fail(SceneQueryFailure::unavailable_field,runtime.first_player->task?0x43af43:0x43af71);
    if(runtime.second_player->task){
        if(!runtime.second_player->task->position)return fail(SceneQueryFailure::unavailable_field,0x43af53);
        if(runtime.first_player->task){views[1]=&*runtime.second_player->task->position;view_count=2;}
        else views[0]=&*runtime.second_player->task->position;
    }
    scene.candidate_count=0;scene.dynamic_count=0;coordinates.push_current_frame();
    if(!runtime.registered_count)return fail(SceneQueryFailure::unavailable_field,0x43afa8);
    if(*runtime.registered_count<0 || static_cast<std::size_t>(*runtime.registered_count)>runtime.registered_entries.size())
        return fail(SceneQueryFailure::unavailable_registry_storage,0x43afb2);
    const auto load=[&](float value){return fp.from_float(value);};
    const auto insert_candidate=[&](const SceneLandEntry& entry){scene.candidates[scene.candidate_count++]=entry;};
    for(std::size_t index=static_cast<std::size_t>(*runtime.registered_count);index>0;--index){
        const auto& registered=runtime.registered_entries[index-1];const auto& entry=registered.land;
        if(!registered.task)return fail(SceneQueryFailure::unavailable_field,0x43afd0);
        if(!registered.task->task)return fail(SceneQueryFailure::null_required_reference,0x43afd3);
        if(!(registered.task->task->orientation.flags&0x100U))continue;
        if(!entry.attributes || !entry.object)return fail(SceneQueryFailure::unavailable_field,0x43afed);
        if(!entry.object->id)return fail(SceneQueryFailure::null_required_reference,0x43aff5);
        const auto found=scene.objects.find(*entry.object->id);if(found==scene.objects.end())return fail(SceneQueryFailure::unresolved_resource,0x43aff5);
        const auto& object=found->second;
        if(!object.model || !object.position)return fail(SceneQueryFailure::unavailable_field,0x43aff5);
        if(!object.model->id)return fail(SceneQueryFailure::null_required_reference,0x43aff8);
        const auto model_it=scene.models.find(*object.model->id);if(model_it==scene.models.end())return fail(SceneQueryFailure::unresolved_resource,0x43aff8);
        const auto& model=model_it->second;if(!model.center || !model.radius)return fail(SceneQueryFailure::unavailable_field,0x43aff8);
        auto model_center=*model.center;
        if(*entry.attributes&0x10000000U){
            if(!object.angles)return fail(SceneQueryFailure::unavailable_field,0x43b01f);
            matrix_site=0x43b01a;coordinates.set_current_unit_matrix();
            if((*object.angles)[2]){matrix_site=0x43b02b;coordinates.rotate_z(coordinates.current_matrix(),(*object.angles)[2],fp);}
            if((*object.angles)[0]){matrix_site=0x43b03c;coordinates.rotate_x(coordinates.current_matrix(),(*object.angles)[0],fp);}
            if((*object.angles)[1]){matrix_site=0x43b04d;coordinates.rotate_y(coordinates.current_matrix(),(*object.angles)[1],fp);}
            matrix_site=0x43b05f;
            coordinates.calculate_point(coordinates.current_matrix(),*model.center,model_center,fp);
        }
        const auto x=fp.add_float_memory(load(model_center.x),object.position->x);
        const auto y=fp.add_float_memory(load(model_center.y),object.position->y);
        const auto z=fp.add_float_memory(load(model_center.z),object.position->z);
        if(!runtime.view_distance)return fail(SceneQueryFailure::unavailable_field,0x43b082);
        bool selected=false;
        for(std::size_t view=0;view<view_count;++view){
            const auto bound=fp.add_float_memory(load(view==0?*runtime.view_distance:constants.second_view_distance),*model.radius);
            const auto dx=fp.to_float(fp.subtract(load(views[view]->x),x));
            const auto dy=fp.to_float(fp.subtract(load(views[view]->y),y));
            const auto dz=fp.to_float(fp.subtract(load(views[view]->z),z));
            const auto square=fp.add(fp.add(fp.multiply_float_memory(load(dz),dz),fp.multiply_float_memory(load(dy),dy)),fp.multiply_float_memory(load(dx),dx));
            if(fp.compare(fp.multiply(bound,bound),square)==X87Comparison::greater){selected=true;break;}
        }
        if(selected){
            if(!entry.task)return fail(SceneQueryFailure::unavailable_field,0x43b203);
            const auto at=static_cast<std::size_t>(*scene.dynamic_count);
            if(scene.dynamic_land_entries.size()<=at)scene.dynamic_land_entries.resize(at+1);
            scene.dynamic_land_entries[at]=entry;++*scene.dynamic_count;insert_candidate(entry);
            if(*scene.dynamic_count==128){
                // Source43b25f pushes7e0018 (not the distinct7e0030 message).
                // Retain exact CP932 bytes; printf has no format substitutions
                // and its return is ignored before pop/static registration.
                static constexpr unsigned char message[]{0x82,0xa0,0x82,0xd3,0x82,0xea,0x82,0xad,0x20,0x97,0xdc,0x30,0x0a};
                (void)std::fwrite(message,1,sizeof(message),stdout);break;
            }
        }
    }
    matrix_site=0x43b109;coordinates.end_frame();scene.static_count=0;
    if(!scene.static_enabled)return fail(SceneQueryFailure::unavailable_field,0x43b10e);
    if(*scene.static_enabled!=1)return {};
    if(!scene.landtable)return fail(SceneQueryFailure::unavailable_field,0x43b12d);
    if(!scene.landtable->id)return {};
    if(table.source_count<=0)return {};
    if(static_cast<std::size_t>(table.source_count)>table.entries.size())return fail(SceneQueryFailure::unavailable_registry_storage,0x43b150);
    for(std::size_t index=0;index<static_cast<std::size_t>(table.source_count);++index){
        const auto& entry=table.entries[index];if(!entry.attributes)return fail(SceneQueryFailure::unavailable_field,0x43b150);
        if(!(*entry.attributes&0x400003U))continue;
        if(!runtime.view_distance)return fail(SceneQueryFailure::unavailable_field,0x43b161);
        if(!entry.center || !entry.radius)return fail(SceneQueryFailure::unavailable_field,0x43b180);
        bool selected=false;
        for(std::size_t view=0;view<view_count;++view){
            const auto dx=fp.to_float(fp.subtract_float_memory(load(entry.center->x),views[view]->x));
            const auto dy=fp.subtract_float_memory(load(entry.center->y),views[view]->y);
            const auto dz=fp.subtract_float_memory(load(entry.center->z),views[view]->z);
            const auto squared=fp.to_float(fp.add(fp.add(fp.multiply(dz,dz),fp.multiply(dy,dy)),fp.multiply_float_memory(load(dx),dx)));
            const auto distance=fp.subtract_float_memory(signed_sqrt_extended(squared,fp),view==0?*runtime.view_distance:constants.second_view_distance);
            if(fp.compare_float_memory(distance,*entry.radius)==X87Comparison::less){selected=true;break;}
        }
        if(selected){
            if(!entry.object)return fail(SceneQueryFailure::unavailable_field,0x43b299);
            const auto at=static_cast<std::size_t>(*scene.static_count);
            if(scene.static_entries.size()<=at)scene.static_entries.resize(at+1);
            scene.static_entries[at]=entry;++*scene.static_count;
            insert_candidate({entry.attributes,entry.object,SceneResourceReference{}});
            if(*scene.static_count==128 || scene.candidate_count==1024)return {};
        }
    }
    return {};
    }catch(const CoordinateError& error){auto result=SceneQueryResult{SceneQueryFailure::invalid_matrix_owner,matrix_site,0,{},{}};result.coordinate_failure=error.failure();return result;}
}
} // namespace sadx
