#include "player/player_mesh_registry.hpp"
#include <cstring>

namespace {
template<class To,class From>To copy_bits(From value){static_assert(sizeof(To)==sizeof(From),"Exact bit-copy width");To result;std::memcpy(&result,&value,sizeof(result));return result;}
constexpr std::uint32_t groups=0x3b43000,nodes=0x3b48550;
std::uint32_t group_ref(const std::optional<sadx::SceneResourceReference>& value){return value&&value->id?groups+static_cast<std::uint32_t>(*value->id)*64:0;}
std::uint32_t node_ref(const std::optional<sadx::SceneResourceReference>& value){return value&&value->id?nodes+static_cast<std::uint32_t>(*value->id)*64:0;}
}
// Output:17 global/return facts,256 source64-byte groups,16384 source64-byte nodes,
//128 bucket pairs,1024 candidate triples,128 dynamic triples,8 matrix frames,
//4 descriptor facts and1024 transformed vertices (mode6 only), initializer EAX.
extern "C" int sadx_native_player_mesh_registry(const std::uint8_t* image,std::size_t size,
    const std::uint8_t* trig,const std::uint8_t* unit,std::uint32_t mode,
    const std::uint32_t* w,std::uint16_t cw,std::uint32_t* out){
    if(!image||!trig||!unit||!w||!out)return -1;
    try {
        auto decoded=sadx::decode_stage_scene_landtable(image,size,1,0);if(!decoded)return -2;auto& land=*decoded.table;
        sadx::PlayerSceneLandscape scene;scene.objects=land.objects;scene.models=land.models;scene.vector_arrays=land.vector_arrays;scene.mesh_set_arrays=land.mesh_set_arrays;scene.index_arrays=land.index_arrays;scene.material_arrays=land.material_arrays;
        scene.static_enabled=1;scene.landtable=sadx::SceneResourceReference{0};
        sadx::MeshRegistryPools pools;for(std::size_t i=0;i<256;++i){pools.groups[i]=i;auto& group=scene.groups[i];group.next=sadx::SceneResourceReference{};group.previous=sadx::SceneMeshGroupBacklink{false,{}};group.object=sadx::SceneResourceReference{};group.head=group.tail=sadx::SceneResourceReference{};group.z_padding=0;group.attributes_at_c=group.source_word_at_18=group.source_word_at_2c=0;group.center_at_30=sadx::NinjaVector{0,0,0};}
        for(std::size_t i=0;i<16384;++i){pools.nodes[i]=i;auto& node=scene.nodes[i];node.next=node.previous=sadx::SceneResourceReference{};node.center=sadx::NinjaVector{0,0,0};node.radius_squared=0;node.triangle_vertices=std::array<sadx::NinjaVector,3>{};node.raw_at_3c=0;}
        sadx::PlayerMeshRegistryState state;state.force_refresh=0;state.base_z=copy_bits<float>(w[2]);state.max_z=0;state.source_word_at_3b42ff8=0;
        sadx::PlayerMeshQueryState query;auto result=sadx::initialize_mesh_registry(scene,state,query,pools);sadx::clear_mesh_registry_buckets(state);
        const auto initializer_outcome=*result.outcome;
        sadx::NinjaMath::SineTable table;std::memcpy(table.data(),trig,4100);sadx::NinjaMatrix identity;std::memcpy(identity.data(),unit,64);
        sadx::NinjaMatrixStack stack(std::vector<sadx::NinjaMatrix>(8,identity),1,0);sadx::PlayerCoordinates coordinates(table,identity,stack);sadx::X87Context fp(cw);sadx::CrtFabsRuntime absolute{1,0,{}};
        const auto ordinal=w[0]%480;const auto object=*land.entries[ordinal].object->id;
        std::optional<sadx::MeshPrimitiveDescriptor> descriptor;
        if(mode==0)result=sadx::produce_mesh_object_sets(scene,state,object,absolute,coordinates,fp);
        else if(mode==1)result=sadx::traverse_mesh_object(scene,state,object,absolute,coordinates,fp);
        else if(mode==2){
            for(std::uint32_t i=0;i<w[0];++i){result=sadx::allocate_mesh_group(scene,state);if(!result)break;}
            for(std::uint32_t i=0;i<w[1];++i){result=sadx::allocate_mesh_node_for_z(scene,state,copy_bits<float>(w[2+i%4]),fp);if(!result)break;}
            if(w[0]&&scene.mesh_groups->id){result=sadx::join_mesh_registry_buckets(scene,state,*scene.mesh_groups->id);if(result&&w[6])result=sadx::release_mesh_group(scene,state,*scene.mesh_groups->id);}
        }else if(mode==6){
            constexpr std::size_t synthetic=1000000;
            scene.models[synthetic].vertices=sadx::SceneResourceReference{synthetic};
            scene.vector_arrays[synthetic]={
                {copy_bits<float>(w[2]),0,0},{0,copy_bits<float>(w[3]),0},{0,0,copy_bits<float>(w[4])},{1,2,3}};
            auto& indices=scene.index_arrays[synthetic];
            if(w[0]==0x8000||w[0]==0xc000)indices.push_back(static_cast<std::uint16_t>(w[1]));
            for(std::size_t i=0;i<1024;++i)indices.push_back(static_cast<std::uint16_t>(i%4));
            descriptor=sadx::MeshPrimitiveDescriptor{};descriptor->kind=w[0];descriptor->vertex_count=w[1];descriptor->model=synthetic;
            result=sadx::transform_mesh_primitive(scene,{synthetic},*descriptor,coordinates,fp);
        }else {
            // Explicit bounded real-record resource view; the record bytes and
            // every model/mesh/index/material remain decoded from the PE.
            land.source_count=static_cast<std::int16_t>(w[0]);
            sadx::SceneRegistrationRuntime runtime;runtime.registered_count=0;
            sadx::PlayerTaskState camera;camera.position=sadx::NinjaVector{copy_bits<float>(w[1]),copy_bits<float>(w[2]),copy_bits<float>(w[3])};
            runtime.camera=sadx::SceneTaskReadView{&camera};runtime.first_player=sadx::SceneTaskReadView{nullptr};runtime.second_player=sadx::SceneTaskReadView{nullptr};runtime.view_distance=copy_bits<float>(w[4]);
            if(mode==3)result=sadx::register_mesh_scene_without_culling(scene,land,runtime);
            else if(mode==4){for(std::uint32_t pass=0;pass<std::max(w[5],1U);++pass){state.force_refresh=1;result=sadx::refresh_mesh_registry(scene,state,{land,runtime,{40},absolute},coordinates,fp);if(!result)break;}}
            else {const auto registered=sadx::register_player_scene_landscape(scene,land,runtime,{40},coordinates,fp);result.failure=registered?sadx::MeshRegistryFailure::none:sadx::MeshRegistryFailure::registration_unavailable;result.source_boundary=registered.source_boundary;}
        }
        if(!result){out[0]=static_cast<std::uint32_t>(result.failure);out[1]=result.source_boundary;out[2]=result.nested_source_boundary;return 1;}
        const auto object_ref=[&](const std::optional<sadx::SceneResourceReference>& ref){return ref&&ref->id?land.object_locations.at(*ref->id).source_virtual_address:0U;};
        out[0]=group_ref(state.free_groups);out[1]=node_ref(state.free_nodes);out[2]=group_ref(scene.mesh_groups);out[3]=copy_bits<std::uint32_t>(*state.base_z);out[4]=copy_bits<std::uint32_t>(*state.max_z);out[5]=*state.force_refresh;out[6]=object_ref(state.forced_object);out[7]=*state.source_word_at_3b42ff8;out[8]=*query.contact_count;out[9]=scene.selected_count;out[10]=static_cast<std::uint32_t>(scene.candidate_count);out[11]=scene.dynamic_count?static_cast<std::uint32_t>(*scene.dynamic_count):0;out[12]=scene.static_count?static_cast<std::uint32_t>(*scene.static_count):0;out[13]=stack.depth();out[14]=fp.control_word();out[15]=fp.exception_flags();
        out[16]=(mode==0||mode==4||mode==6)?*result.outcome:mode==1?object_ref(result.returned_reference):0U;
        std::size_t at=17;
        for(std::size_t i=0;i<256;++i){const auto& g=scene.groups.at(i);out[at]=group_ref(g.next);out[at+1]=g.previous->registry_head?~0x3c48550U:g.previous->group?groups+static_cast<std::uint32_t>(*g.previous->group)*64:0;out[at+2]=object_ref(g.object);out[at+3]=*g.attributes_at_c;out[at+4]=node_ref(g.head);out[at+5]=node_ref(g.tail);out[at+6]=*g.source_word_at_18;out[at+7]=0;out[at+8]=0;out[at+9]=g.selected_count;out[at+10]=copy_bits<std::uint32_t>(*g.z_padding);out[at+11]=*g.source_word_at_2c;out[at+12]=copy_bits<std::uint32_t>(g.center_at_30->x);out[at+13]=copy_bits<std::uint32_t>(g.center_at_30->y);out[at+14]=copy_bits<std::uint32_t>(g.center_at_30->z);out[at+15]=0;at+=16;}
        for(std::size_t i=0;i<16384;++i){const auto& n=scene.nodes.at(i);out[at]=node_ref(n.next);out[at+1]=node_ref(n.previous);out[at+2]=copy_bits<std::uint32_t>(n.center->x);out[at+3]=copy_bits<std::uint32_t>(n.center->y);out[at+4]=copy_bits<std::uint32_t>(n.center->z);out[at+5]=copy_bits<std::uint32_t>(*n.radius_squared);for(std::size_t j=0;j<3;++j){out[at+6+j*3]=copy_bits<std::uint32_t>((*n.triangle_vertices)[j].x);out[at+7+j*3]=copy_bits<std::uint32_t>((*n.triangle_vertices)[j].y);out[at+8+j*3]=copy_bits<std::uint32_t>((*n.triangle_vertices)[j].z);}out[at+15]=*n.raw_at_3c;at+=16;}
        for(const auto& bucket:state.buckets){out[at++]=node_ref(bucket.head);out[at++]=node_ref(bucket.tail);}
        for(std::size_t i=0;i<1024;++i){const auto& entry=scene.candidates[i];out[at++]=entry&&entry->attributes?*entry->attributes:0;out[at++]=entry?object_ref(entry->object):0;out[at++]=0;}
        for(std::size_t i=0;i<128;++i){const auto* entry=i<scene.dynamic_land_entries.size()?&scene.dynamic_land_entries[i]:nullptr;out[at++]=entry&&entry->attributes?*entry->attributes:0;out[at++]=entry?object_ref(entry->object):0;out[at++]=0;}
        for(const auto& frame:stack.slots())for(const auto value:frame)out[at++]=copy_bits<std::uint32_t>(value);
        out[at++]=descriptor?descriptor->kind:0;out[at++]=descriptor?descriptor->vertex_count:0;out[at++]=descriptor?static_cast<std::uint32_t>(descriptor->index_cursor):0;out[at++]=descriptor?*result.outcome:0;
        for(std::size_t i=0;i<1024;++i){const auto* value=descriptor&&descriptor->transformed_vertices[i]?&*descriptor->transformed_vertices[i]:nullptr;out[at++]=value?copy_bits<std::uint32_t>(value->x):0;out[at++]=value?copy_bits<std::uint32_t>(value->y):0;out[at++]=value?copy_bits<std::uint32_t>(value->z):0;}out[at++]=initializer_outcome;return 0;
    }catch(const sadx::X87Error&){return -3;}catch(const sadx::CoordinateError&){return -4;}catch(const std::out_of_range&){return -5;}
}
extern "C" const char* sadx_native_player_mesh_registry_compiler(){return "Clang " __clang_version__ "; ARM64 C++17 O2 source-native fixed-pool mesh producer";}
