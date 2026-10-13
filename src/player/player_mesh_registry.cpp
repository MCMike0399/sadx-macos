#include "player/player_mesh_registry.hpp"
#include "cpu/crt_integer.hpp"
#include <algorithm>
#include <cstring>
#include <cstdio>
#include <unordered_set>

namespace sadx {
namespace {
template<class To,class From>To copy_bits(From value){static_assert(sizeof(To)==sizeof(From),"Exact bit-copy width");To result;std::memcpy(&result,&value,sizeof(result));return result;}
struct Missing { MeshRegistryFailure why; std::uint32_t site; };
template<class T>const T& need(const std::optional<T>& value,std::uint32_t site){
    if(!value)throw Missing{MeshRegistryFailure::unavailable_field,site};return *value;
}
std::size_t nonnull(const SceneResourceReference& value,std::uint32_t site){
    if(!value.id)throw Missing{MeshRegistryFailure::null_required_reference,site};return *value.id;
}
template<class M>auto& resource(M& map,std::size_t id,std::uint32_t site){
    const auto found=map.find(id);if(found==map.end())throw Missing{MeshRegistryFailure::unresolved_resource,site};return found->second;
}
MeshRegistryResult failed(Missing missing){MeshRegistryResult result;result.failure=missing.why;result.source_boundary=missing.site;return result;}
MeshRegistryResult returned(SceneResourceReference ref){MeshRegistryResult result;result.returned_reference=ref;return result;}
MeshRegistryResult outcome(std::uint32_t value){MeshRegistryResult result;result.outcome=value;return result;}
MeshRegistryResult invalid_matrix(std::uint32_t site,const CoordinateError& error){auto result=failed({MeshRegistryFailure::invalid_matrix_owner,site});result.coordinate_failure=error.failure();return result;}
void propagate(const MeshRegistryResult& result){if(!result)throw result;}
bool same(const SceneResourceReference& a,const SceneResourceReference& b){return a.id==b.id;}
}

MeshRegistryResult initialize_mesh_registry(PlayerSceneLandscape& scene,PlayerMeshRegistryState& state,PlayerMeshQueryState& query,const MeshRegistryPools& pools){
    std::unordered_set<std::size_t> ids;
    for(const auto id:pools.groups)if(!ids.insert(id).second)return failed({MeshRegistryFailure::invalid_pool_layout,0x451030});
    ids.clear();for(const auto id:pools.nodes)if(!ids.insert(id).second)return failed({MeshRegistryFailure::invalid_pool_layout,0x451030});
    scene.selected_count=0;query.contact_count=0;scene.mesh_groups=SceneResourceReference{};state.forced_object=SceneResourceReference{};
    for(const auto id:pools.groups)scene.groups.try_emplace(id).first->second.object=SceneResourceReference{};
    for(const auto id:pools.nodes)scene.nodes.try_emplace(id).first->second.radius_squared=0.0F;
    state.free_nodes=SceneResourceReference{pools.nodes[0]};
    for(std::size_t i=0;i<pools.nodes.size();++i)scene.nodes.at(pools.nodes[i]).next=SceneResourceReference{i+1<pools.nodes.size()?std::optional<std::size_t>{pools.nodes[i+1]}:std::nullopt};
    state.free_groups=SceneResourceReference{pools.groups[0]};
    for(std::size_t i=0;i<pools.groups.size();++i)scene.groups.at(pools.groups[i]).next=SceneResourceReference{i+1<pools.groups.size()?std::optional<std::size_t>{pools.groups[i+1]}:std::nullopt};
    scene.mesh_groups=SceneResourceReference{};
    // 451030 does not initialize force-refresh, buckets, or other payload bytes.
    return outcome(1); // Source4510cb MOV EAX,1 after both pool link owners.
}
void clear_mesh_registry_buckets(PlayerMeshRegistryState& state){for(auto& bucket:state.buckets){bucket.head=SceneResourceReference{};bucket.tail=SceneResourceReference{};}}
MeshRegistryResult allocate_mesh_group(PlayerSceneLandscape& scene,PlayerMeshRegistryState& state){
    try {
        const auto id=nonnull(need(state.free_groups,0x451796),0x4517a1);auto& group=resource(scene.groups,id,0x4517a1);
        const auto next=need(group.next,0x4517a1);if(!next.id)return returned({});
        state.free_groups=next;const auto old=need(scene.mesh_groups,0x4517b8);group.next=old;
        if(old.id)resource(scene.groups,*old.id,0x4517d7).previous=SceneMeshGroupBacklink{false,id};
        scene.mesh_groups=SceneResourceReference{id};group.previous=SceneMeshGroupBacklink{true,{}};return returned({id});
    }catch(Missing missing){return failed(missing);}
}
MeshRegistryResult release_mesh_group(PlayerSceneLandscape& scene,PlayerMeshRegistryState& state,std::size_t id){
    try {
        auto& group=resource(scene.groups,id,0x451809);const auto head=need(group.head,0x451809),tail=need(group.tail,0x451812);
        if(head.id&&tail.id){resource(scene.nodes,*tail.id,0x45182d).next=need(state.free_nodes,0x451827);state.free_nodes=head;}
        group.object=SceneResourceReference{};
        const auto next=need(group.next,0x451869);const auto previous=need(group.previous,0x451871);
        if(next.id)resource(scene.groups,*next.id,0x451883).previous=previous;
        if(previous.registry_head)scene.mesh_groups=next;
        else resource(scene.groups,nonnull({previous.group},0x4518a8),0x4518a8).next=next;
        group.next=need(state.free_groups,0x4518ad);state.free_groups=SceneResourceReference{id};return {};
    }catch(Missing missing){return failed(missing);}
}
MeshRegistryResult allocate_mesh_node(PlayerSceneLandscape& scene,PlayerMeshRegistryState& state,std::size_t bucket_index){
    try {
        if(bucket_index>=state.buckets.size())throw Missing{MeshRegistryFailure::unavailable_array_storage,0x4521c0};
        const auto id=nonnull(need(state.free_nodes,0x4521c6),0x4521d1);auto& node=resource(scene.nodes,id,0x4521d1);
        const auto next=need(node.next,0x4521d1);if(!next.id)return returned({});
        state.free_nodes=next;auto& bucket=state.buckets[bucket_index];const auto old=need(bucket.head,0x4521eb);node.next=old;
        if(old.id)resource(scene.nodes,*old.id,0x452204).previous=SceneResourceReference{id};else bucket.tail=SceneResourceReference{id};
        bucket.head=SceneResourceReference{id};node.previous=SceneResourceReference{};return returned({id});
    }catch(Missing missing){return failed(missing);}
}
MeshRegistryResult allocate_mesh_node_for_z(PlayerSceneLandscape& scene,PlayerMeshRegistryState& state,float z,X87Context& fp){
    const auto bits=crt_ftol_low32(fp.from_float(z),fp);
    // Explicit x86 SAR and modulo ADD; host signed shifts/overflow are avoided.
    const auto shifted=(bits>>4)|((bits&0x80000000U)?0xf0000000U:0U);
    const auto bucket=copy_bits<std::int32_t>(shifted+64U);
    return allocate_mesh_node(scene,state,static_cast<std::size_t>(std::clamp(bucket,0,127)));
}
MeshRegistryResult join_mesh_registry_buckets(PlayerSceneLandscape& scene,PlayerMeshRegistryState& state,std::size_t group_id){
    try {
        std::size_t first=0;SceneResourceReference last_read;
        while(first<128){last_read=need(state.buckets[first].head,0x451938);if(last_read.id)break;++first;}
        auto& group=resource(scene.groups,group_id,first==128?0x451959:0x451977);
        if(first==128){group.head=SceneResourceReference{};group.tail=SceneResourceReference{};return returned({});}
        group.head=last_read;auto tail=need(state.buckets[first].tail,0x45197d);
        for(std::size_t i=first+1;i<128;++i){last_read=need(state.buckets[i].head,0x4519bd);if(!last_read.id)continue;
            resource(scene.nodes,nonnull(tail,0x4519ce),0x4519ce).next=last_read;
            resource(scene.nodes,*last_read.id,0x4519d6).previous=tail;tail=need(state.buckets[i].tail,0x4519dc);
        }
        group.tail=tail;return returned(last_read); // Source EAX is the final examined bucket's head.
    }catch(Missing missing){return failed(missing);}
}

MeshRegistryResult transform_mesh_primitive(const PlayerSceneLandscape& scene,const SceneResourceReference& index_ref,
    MeshPrimitiveDescriptor& descriptor,PlayerCoordinates& coordinates,X87Context& fp){
    std::uint32_t matrix_site=0x4522d2;
    try {
        std::uint32_t flip=0;
        if(descriptor.kind!=0&&descriptor.kind!=0x4000&&descriptor.kind!=0x8000&&descriptor.kind!=0xc000)return outcome(0);
        const auto& indices=resource(scene.index_arrays,nonnull(index_ref,0x4522c8),0x4522c8);
        if(descriptor.kind==0x8000||descriptor.kind==0xc000){
            if(descriptor.index_cursor>=indices.size())throw Missing{MeshRegistryFailure::unavailable_array_storage,0x4522e4};
            const auto header=indices[descriptor.index_cursor];descriptor.vertex_count=header&0x7fffU;flip=(header>>15)&1U;
            descriptor.vertex_count=std::min(descriptor.vertex_count,1024U);++descriptor.index_cursor;matrix_site=0x452386;
        }
        for(std::uint32_t i=0;i<descriptor.vertex_count;++i){
            if(i>=1024||descriptor.index_cursor>=indices.size())throw Missing{MeshRegistryFailure::unavailable_array_storage,descriptor.kind<0x8000?0x4522c8U:0x45237cU};
            const auto& model=resource(scene.models,descriptor.model,descriptor.kind<0x8000?0x4522ce:0x452382);
            const auto& vertices=resource(scene.vector_arrays,nonnull(need(model.vertices,matrix_site),matrix_site),matrix_site);
            const auto index=copy_bits<std::int16_t>(indices[descriptor.index_cursor]);
            if(index<0||static_cast<std::size_t>(index)>=vertices.size())throw Missing{MeshRegistryFailure::unavailable_array_storage,matrix_site};
            NinjaVector transformed;coordinates.calculate_point(coordinates.current_matrix(),vertices[static_cast<std::size_t>(index)],transformed,fp);
            descriptor.transformed_vertices[i]=transformed;++descriptor.index_cursor;
        }
        return outcome(flip);
    }catch(Missing missing){return failed(missing);}catch(const CoordinateError& error){return invalid_matrix(matrix_site,error);}
}

MeshRegistryResult produce_mesh_object_sets(PlayerSceneLandscape& scene,PlayerMeshRegistryState& state,std::size_t object_id,
    CrtFabsRuntime& absolute_math,PlayerCoordinates& coordinates,X87Context& fp){
    MeshRegistryResult result;result.object=object_id;
    try {
        const auto& object=resource(scene.objects,object_id,0x451ae0);
        const auto model_id=nonnull(need(object.model,0x451ae0),0x451aef);const auto& model=resource(scene.models,model_id,0x451aef);
        const auto set_count=need(model.meshset_count,0x451aef);const auto sets_ref=need(model.meshsets,0x451afc);
        if(set_count==0){result.outcome=1;return result;}
        const auto& sets=resource(scene.mesh_set_arrays,nonnull(sets_ref,0x451b2f),0x451b2f);
        MeshPrimitiveDescriptor descriptor{};descriptor.model=model_id;
        for(std::size_t set_index=0;set_index<set_count;++set_index){result.meshset=set_index;
            if(set_index>=sets.size())throw Missing{MeshRegistryFailure::unavailable_array_storage,0x451b2f};const auto& set=sets[set_index];
            const auto type=need(set.type_and_material,0x451b2f);descriptor.kind=type&0xc000U;
            if(descriptor.kind==0)descriptor.vertex_count=3;else if(descriptor.kind==0x4000)descriptor.vertex_count=4;
            const auto primitive_count=need(set.mesh_count,0x451bab);const auto indices=need(set.indices,0x451bbb);descriptor.index_cursor=0;
            for(std::size_t primitive=0;primitive<primitive_count;++primitive){result.primitive=primitive;
                const auto transformed=transform_mesh_primitive(scene,indices,descriptor,coordinates,fp);if(!transformed){auto failure=transformed;failure.object=result.object;failure.meshset=set_index;failure.primitive=primitive;return failure;}
                for(std::size_t triangle=0;triangle+2<descriptor.vertex_count;++triangle){result.triangle=triangle;
                    auto vertices=std::array<NinjaVector,3>{{need(descriptor.transformed_vertices[triangle],0x451c48),need(descriptor.transformed_vertices[triangle+1],0x451c56),need(descriptor.transformed_vertices[triangle+2],0x451c65)}};
                    const float third=copy_bits<float>(0x3eaaa64cU);
                    const auto centroid=[&](float a,float b,float c){return fp.to_float(fp.multiply_float_memory(fp.add_float_memory(fp.add_float_memory(fp.from_float(a),b),c),third));};
                    const NinjaVector center{centroid(vertices[0].x,vertices[1].x,vertices[2].x),centroid(vertices[0].y,vertices[1].y,vertices[2].y),centroid(vertices[0].z,vertices[1].z,vertices[2].z)};
                    const auto delta=fp.to_float(fp.subtract_float_memory(fp.from_float(center.z),need(state.base_z,0x451d32)));
                    const auto allocation=allocate_mesh_node_for_z(scene,state,delta,fp);if(!allocation)return allocation;
                    const auto node_ref=need(allocation.returned_reference,0x451d44);if(!node_ref.id){result.outcome=0;return result;}
                    result.node=*node_ref.id;auto& node=resource(scene.nodes,*node_ref.id,0x451d7a);
                    node.triangle_vertices=vertices;if((triangle+*transformed.outcome)&1U)std::swap((*node.triangle_vertices)[1],(*node.triangle_vertices)[2]);
                    std::array<float,3> spans;
                    constexpr std::array<std::uint32_t,3> fabs_sites{{0x451e93,0x451eb9,0x451edf}};
                    for(std::size_t edge=0;edge<3;++edge){
                        const auto a=(edge+1)%3,b=edge;
                        const auto argument=fp.to_double(fp.subtract_float_memory(fp.from_float(vertices[a].z),vertices[b].z));
                        result.fabs=crt_fabs(argument,absolute_math,fp);
                        if(!*result.fabs){result.failure=MeshRegistryFailure::fabs_unavailable;result.source_boundary=fabs_sites[edge];result.nested_source_boundary=result.fabs->source_boundary;return result;}
                        (void)fp.to_float(*result.fabs->value);spans[edge]=fp.to_float(*result.fabs->value);
                    }
                    auto span=spans[0];for(std::size_t i=1;i<3;++i)if(fp.compare_float_memory(fp.from_float(spans[i]),span)==X87Comparison::greater)span=spans[i];
                    if(fp.compare_float_memory(fp.from_float(span),need(state.max_z,0x451f37))==X87Comparison::greater)state.max_z=span;
                    for(auto& vertex:vertices){vertex.x=fp.to_float(fp.subtract_float_memory(fp.from_float(vertex.x),center.x));vertex.y=fp.to_float(fp.subtract_float_memory(fp.from_float(vertex.y),center.y));vertex.z=fp.to_float(fp.subtract_float_memory(fp.from_float(vertex.z),center.z));}
                    node.center=center;
                    const auto square=[&](const NinjaVector& v){const auto z=fp.multiply_float_memory(fp.from_float(v.z),v.z);const auto y=fp.multiply_float_memory(fp.from_float(v.y),v.y);const auto x=fp.multiply_float_memory(fp.from_float(v.x),v.x);return fp.to_float(fp.add(z,fp.add(y,x)));};
                    float radius=square(vertices[0]);for(std::size_t i=1;i<3;++i){const auto next=square(vertices[i]);if(fp.compare_float_memory(fp.from_float(radius),next)==X87Comparison::less)radius=next;}
                    node.radius_squared=radius;const auto materials=need(model.materials,0x4520fb);
                    if(!materials.id)node.raw_at_3c=0;
                    else {const auto& array=resource(scene.material_arrays,*materials.id,0x45212a);const auto index=type&0x3fffU;if(index>=array.size())throw Missing{MeshRegistryFailure::unavailable_array_storage,0x45212a};node.raw_at_3c=array[index].words[4];}
                }
            }
        }
        result.outcome=1;return result;
    }catch(Missing missing){result.failure=missing.why;result.source_boundary=missing.site;return result;}
}
namespace {
void transform_object(const SceneGeometryObject& object,PlayerCoordinates& coordinates,X87Context& fp,std::uint32_t& site,bool refresh){
    site=refresh?0x4515c4:0x451a13;coordinates.translate_matrix(coordinates.current_matrix(),need(object.position,site),fp);
    const auto angles=need(object.angles,refresh?0x4515cf:0x451a1b);
    if(angles[2]){site=refresh?0x4515e1:0x451a2a;coordinates.rotate_z(coordinates.current_matrix(),static_cast<std::uint16_t>(angles[2]),fp);}
    if(angles[1]){site=refresh?0x4515fe:0x451a41;coordinates.rotate_y(coordinates.current_matrix(),static_cast<std::uint16_t>(angles[1]),fp);}
    if(angles[0]){site=refresh?0x45161b:0x451a58;coordinates.rotate_x(coordinates.current_matrix(),static_cast<std::uint16_t>(angles[0]),fp);}
    site=refresh?0x45162b:0x451a65;scale_mesh_matrix_vector(coordinates.current_matrix(),need(object.scale,site),fp);
}
MeshRegistryResult traverse(PlayerSceneLandscape& scene,PlayerMeshRegistryState& state,std::size_t id,CrtFabsRuntime& math,
    PlayerCoordinates& coordinates,X87Context& fp,std::unordered_set<std::size_t>& ancestry){
    std::uint32_t site=0x451a06;
    try {
        if(!ancestry.insert(id).second)return failed({MeshRegistryFailure::cyclic_object_graph,0x451a85});
        coordinates.push_current_frame();const auto& object=resource(scene.objects,id,0x451a0b);transform_object(object,coordinates,fp,site,false);
        auto child=need(object.child,0x451a6d);std::unordered_set<std::size_t> siblings;
        while(child.id){if(!siblings.insert(*child.id).second)return failed({MeshRegistryFailure::cyclic_object_graph,0x451a85});
            const auto result=traverse(scene,state,*child.id,math,coordinates,fp,ancestry);if(!result)return result;child=need(result.returned_reference,0x451a8d);
        }
        const auto result=produce_mesh_object_sets(scene,state,id,math,coordinates,fp);if(!result)return result;
        site=*result.outcome==0?0x451aaf:0x451abd;coordinates.end_frame();ancestry.erase(id);
        if(*result.outcome==0)return returned({});return returned(need(object.sibling,0x451ac5));
    }catch(Missing missing){return failed(missing);}catch(const CoordinateError& error){return invalid_matrix(site,error);}
}
}
MeshRegistryResult traverse_mesh_object(PlayerSceneLandscape& scene,PlayerMeshRegistryState& state,std::size_t id,
    CrtFabsRuntime& math,PlayerCoordinates& coordinates,X87Context& fp){std::unordered_set<std::size_t> ancestry;return traverse(scene,state,id,math,coordinates,fp,ancestry);}

MeshRegistryResult register_mesh_scene_without_culling(PlayerSceneLandscape& scene,const DecodedSceneLandTable& table,
    const SceneRegistrationRuntime& runtime){
    try {
        const auto count=need(runtime.registered_count,0x43b581);scene.candidate_count=0;scene.dynamic_count=0;
        for(std::int32_t index=count;index>0;--index){
            if(static_cast<std::size_t>(index)>runtime.registered_entries.size())throw Missing{MeshRegistryFailure::unavailable_array_storage,0x43b5be};
            const auto& entry=runtime.registered_entries[static_cast<std::size_t>(index-1)].land;
            // MOV copies preserve unknown leaf facts rather than manufacturing
            // a value; the first consumer that needs such a fact reports it.
            scene.candidates[static_cast<std::size_t>(scene.candidate_count)]=entry;
            const auto at=static_cast<std::size_t>(*scene.dynamic_count);if(scene.dynamic_land_entries.size()<=at)scene.dynamic_land_entries.resize(at+1);
            scene.dynamic_land_entries[at]=entry;++*scene.dynamic_count;++scene.candidate_count;
            if(*scene.dynamic_count==128){
                static constexpr unsigned char message[]{0x82,0xa0,0x82,0xd3,0x82,0xea,0x82,0xad,0x20,0x97,0xdc,0x31,0x0a};
                (void)std::fwrite(message,1,sizeof(message),stdout);break;
            }
            if(scene.candidate_count>=1024)break;
        }
        const auto enabled=need(scene.static_enabled,0x43b61c);scene.static_count=0;if(enabled!=1)return {};
        const auto land=need(scene.landtable,0x43b638);if(!land.id||table.source_count<=0)return {};
        for(std::int32_t index=0;index<table.source_count;++index){
            if(static_cast<std::size_t>(index)>=table.entries.size())throw Missing{MeshRegistryFailure::unavailable_array_storage,0x43b651};
            const auto& entry=table.entries[static_cast<std::size_t>(index)];const auto attributes=need(entry.attributes,0x43b651);if(!(attributes&0x400003U))continue;
            const auto at=static_cast<std::size_t>(*scene.static_count);if(scene.static_entries.size()<=at)scene.static_entries.resize(at+1);scene.static_entries[at]=entry;
            scene.candidates[static_cast<std::size_t>(scene.candidate_count)]=SceneLandEntry{attributes,entry.object,SceneResourceReference{}};
            ++*scene.static_count;++scene.candidate_count;
            if(*scene.static_count>=128||scene.candidate_count>=1024)break;
        }
        return {};
    }catch(Missing missing){return failed(missing);}
}

MeshRegistryResult refresh_mesh_registry(PlayerSceneLandscape& scene,PlayerMeshRegistryState& state,
    const MeshRegistryRefreshRuntime& runtime,PlayerCoordinates& coordinates,X87Context& fp){
    MeshRegistryResult result;std::uint32_t site=0x45150a;
    try {
        if(need(state.force_refresh,0x4511d9)==0){
            result.registration=register_player_scene_landscape(scene,runtime.table,runtime.registration,runtime.registration_constants,coordinates,fp);
            if(!*result.registration){result.failure=MeshRegistryFailure::registration_unavailable;result.source_boundary=0x4511e2;result.nested_source_boundary=result.registration->source_boundary;return result;}
        }else {const auto registration=register_mesh_scene_without_culling(scene,runtime.table,runtime.registration);if(!registration)return registration;state.force_refresh=0;}
        std::vector<SceneLandEntry> builds;
        const auto entry_at=[&](std::size_t index)->const SceneLandEntry&{if(index>=scene.candidates.size()||!scene.candidates[index])throw Missing{MeshRegistryFailure::unavailable_array_storage,0x45127a};return *scene.candidates[index];};
        for(std::int32_t index=0;index<std::min<std::int32_t>(scene.candidate_count,256);++index){
            const auto& entry=entry_at(static_cast<std::size_t>(index));const auto attributes=need(entry.attributes,0x45127a);const auto object=need(entry.object,0x4512e4);bool scheduled=(attributes&0x08000000U)!=0;
            if(!scheduled){
                if(same(need(state.forced_object,0x4512de),object)){state.forced_object=SceneResourceReference{};scheduled=true;}
                else {auto link=need(scene.mesh_groups,0x451350);std::unordered_set<std::size_t> visited;bool present=false;
                    while(link.id){if(!visited.insert(*link.id).second)throw Missing{MeshRegistryFailure::cyclic_object_graph,0x451363};const auto& group=resource(scene.groups,*link.id,0x451380);
                        if(same(need(group.object,0x451380),object)){present=true;break;}link=need(group.next,0x451363);
                    }scheduled=!present;
                }
            }
            if(scheduled)builds.push_back(entry);
        }
        if(builds.size()!=256){
            auto link=need(scene.mesh_groups,0x451411);std::unordered_set<std::size_t> visited;
            while(link.id){const auto id=*link.id;if(!visited.insert(id).second)throw Missing{MeshRegistryFailure::cyclic_object_graph,0x45141c};auto& group=resource(scene.groups,id,0x45147f);bool keep=false;
                for(std::int32_t index=0;index<scene.candidate_count;++index){const auto& entry=entry_at(static_cast<std::size_t>(index));
                    if(!same(need(group.object,0x45147f),need(entry.object,0x451482)))continue;
                    const auto attributes=need(entry.attributes,0x45148d);if(!(attributes&0x08000000U))keep=!same(need(state.forced_object,0x45149f),*entry.object);break;
                }
                const auto next=need(group.next,0x4514ca);if(!keep)propagate(release_mesh_group(scene,state,id));link=next;
            }
        }
        coordinates.push_unit_frame();
        for(const auto& entry:builds){
            const auto allocation=allocate_mesh_group(scene,state);if(!allocation)return allocation;const auto group_ref=need(allocation.returned_reference,0x451588);if(!group_ref.id)break;
            const auto group_id=*group_ref.id;auto& group=resource(scene.groups,group_id,0x45158e);clear_mesh_registry_buckets(state);
            site=0x4515a6;coordinates.push_unit_frame();const auto object_id=nonnull(need(entry.object,0x4515b1),0x4515bf);result.object=object_id;
            const auto& object=resource(scene.objects,object_id,0x4515bf);transform_object(object,coordinates,fp,site,true);
            const auto& model=resource(scene.models,nonnull(need(object.model,0x451636),0x451639),0x451639);auto center=need(model.center,0x451639);
            site=0x451675;coordinates.calculate_point(coordinates.current_matrix(),center,center,fp);state.base_z=center.z;group.center_at_30=center;
            site=0x4516b8;coordinates.end_frame();state.max_z=0.0F;state.source_word_at_3b42ff8=0;
            group.object=*entry.object;group.attributes_at_c=need(entry.attributes,0x4516ee);group.head=SceneResourceReference{};group.source_word_at_18=0;group.selected_begin.reset();group.selected_count=0;
            // The executable ignores completed source return0 from a depleted
            // node pool, then joins the already produced partial geometry.
            const auto production=traverse_mesh_object(scene,state,object_id,runtime.absolute_math,coordinates,fp);if(!production)return production;
            const auto joined=join_mesh_registry_buckets(scene,state,group_id);if(!joined)return joined;
            group.z_padding=need(state.max_z,0x45175d);group.source_word_at_2c=need(state.source_word_at_3b42ff8,0x45176c);
        }
        site=0x45177e;coordinates.end_frame();result.outcome=1;return result;
    }catch(Missing missing){result.failure=missing.why;result.source_boundary=missing.site;return result;}
    catch(const MeshRegistryResult& failure){return failure;}
    catch(const CoordinateError& error){return invalid_matrix(site,error);}
}

} // namespace sadx
