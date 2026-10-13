#include "player/player_mesh_registry.hpp"
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <unordered_set>

namespace {
void require(bool condition,const char* text){if(!condition)throw std::runtime_error(text);}
template<class T>T exact(const char* path){T value;std::ifstream file(path,std::ios::binary);file.read(reinterpret_cast<char*>(&value),sizeof(value));require(file.gcount()==sizeof(value)&&file.peek()==std::char_traits<char>::eof(),"Exact source asset required");return value;}
std::vector<std::uint8_t> bytes(const char* path){std::ifstream file(path,std::ios::binary);return {std::istreambuf_iterator<char>(file),{}};}
sadx::MeshRegistryPools pools(){sadx::MeshRegistryPools result;for(std::size_t i=0;i<256;++i)result.groups[i]=i;for(std::size_t i=0;i<16384;++i)result.nodes[i]=i;return result;}
void tests(const sadx::NinjaMath::SineTable& sine,const sadx::NinjaMatrix& unit,const std::vector<std::uint8_t>& image){
    sadx::PlayerSceneLandscape scene;sadx::PlayerMeshRegistryState registry;sadx::PlayerMeshQueryState query;query.contact_count=99;registry.force_refresh=47;
    scene.nodes[2].raw_at_3c=0x12345678;scene.groups[2].source_word_at_2c=0xabcdef01;
    const auto initialized=sadx::initialize_mesh_registry(scene,registry,query,pools());
    require(static_cast<bool>(initialized)&&initialized.outcome&&*initialized.outcome==1,"Actual pool initializer returns source1");
    require(*query.contact_count==0&&*registry.force_refresh==47&&*scene.nodes[2].raw_at_3c==0x12345678&&*scene.groups[2].source_word_at_2c==0xabcdef01&&!scene.groups[0].previous&&!registry.buckets[0].head,"Only actual initializer writes become known");
    for(std::size_t i=0;i<255;++i){const auto allocated=sadx::allocate_mesh_group(scene,registry);require(static_cast<bool>(allocated)&&allocated.returned_reference->id==i,"Source255 usable groups preserve order");scene.groups[i].head=sadx::SceneResourceReference{};scene.groups[i].tail=sadx::SceneResourceReference{};}
    auto allocated=sadx::allocate_mesh_group(scene,registry);require(static_cast<bool>(allocated)&&!allocated.returned_reference->id&&registry.free_groups->id==255,"Final group is an unconsumed sentinel");
    require(static_cast<bool>(sadx::release_mesh_group(scene,registry,254))&&scene.mesh_groups->id==253&&scene.groups[253].previous->registry_head,"Encoded head backlink repairs the actual owner");
    require(static_cast<bool>(sadx::release_mesh_group(scene,registry,100))&&scene.groups[101].next->id==99&&scene.groups[99].previous->group==101,"Interior unlink updates both links");
    allocated=sadx::allocate_mesh_group(scene,registry);require(allocated.returned_reference->id==100,"Freed group is reused first");
    sadx::clear_mesh_registry_buckets(registry);sadx::X87Context fp(0x027f);
    for(std::size_t i=0;i<16383;++i){const auto node=sadx::allocate_mesh_node(scene,registry,i%128);require(static_cast<bool>(node)&&node.returned_reference->id==i,"Source16383 usable nodes preserve order");}
    auto node=sadx::allocate_mesh_node(scene,registry,0);require(static_cast<bool>(node)&&!node.returned_reference->id&&registry.free_nodes->id==16383,"Final node is an unconsumed sentinel");
    require(static_cast<bool>(sadx::join_mesh_registry_buckets(scene,registry,100)),"Join all original buckets");
    const auto head=scene.groups[100].head->id,tail=scene.groups[100].tail->id;require(head&&tail,"Joined chain exists");
    require(static_cast<bool>(sadx::release_mesh_group(scene,registry,100))&&registry.free_nodes->id==head&&scene.nodes[*tail].next->id==16383,"Release returns the entire chain with its source sentinel");
    require(static_cast<bool>(sadx::initialize_mesh_registry(scene,registry,query,pools())),"Reinitialize retains other bytes");sadx::clear_mesh_registry_buckets(registry);
    registry.free_nodes.reset();node=sadx::allocate_mesh_node(scene,registry,0);require(node.failure==sadx::MeshRegistryFailure::unavailable_field,"Unknown free owner is not allocation exhaustion");
    const auto decoded=sadx::decode_stage_scene_landtable(image.data(),image.size(),1,0);require(static_cast<bool>(decoded),"Actual source ECoast resource decode");const auto& land=*decoded.table;
    scene.objects=land.objects;scene.models=land.models;scene.vector_arrays=land.vector_arrays;scene.mesh_set_arrays=land.mesh_set_arrays;scene.index_arrays=land.index_arrays;scene.material_arrays=land.material_arrays;
    require(static_cast<bool>(sadx::initialize_mesh_registry(scene,registry,query,pools())),"Initialize real production pool");sadx::clear_mesh_registry_buckets(registry);registry.base_z=0;registry.max_z=0;
    sadx::NinjaMatrixStack stack(std::vector<sadx::NinjaMatrix>(8,unit),1,0);sadx::PlayerCoordinates coordinates(sine,unit,stack);sadx::CrtFabsRuntime absolute{1,0,{}};
    const auto object=*land.entries[0].object->id;
    const auto production=sadx::traverse_mesh_object(scene,registry,object,absolute,coordinates,fp);require(static_cast<bool>(production)&&stack.depth()==1&&registry.free_nodes->id&&*registry.free_nodes->id>0,"Real arrays produce native triangles and restore source frames");
    require(scene.nodes[0].triangle_vertices&&scene.nodes[0].center&&scene.nodes[0].radius_squared&&scene.nodes[0].raw_at_3c,"Produced polygon fields have actual resource facts");
    const auto model=*scene.objects.at(object).model->id;auto& vertices=scene.models.at(model).vertices;vertices.reset();
    const auto incomplete=sadx::produce_mesh_object_sets(scene,registry,object,absolute,coordinates,fp);require(incomplete.failure==sadx::MeshRegistryFailure::unavailable_field,"Missing vertex owner cannot produce successful empty geometry");
    scene.static_enabled=1;scene.landtable=sadx::SceneResourceReference{0};sadx::SceneRegistrationRuntime runtime;runtime.registered_count=0;
    scene.dynamic_land_entries.resize(2);scene.dynamic_land_entries[1].attributes=0xfeed;
    require(static_cast<bool>(sadx::register_mesh_scene_without_culling(scene,land,runtime))&&scene.candidate_count==128&&*scene.static_count==128&&*scene.dynamic_land_entries[1].attributes==0xfeed&&scene.candidates[0]->object->id==land.entries[0].object->id,"Static append mutates3b2f720 only, preserving3b32728 tail");
}
}
int main(int argc,char** argv){try{require(argc==4,"usage: mesh_registry_test QUARTER_TABLE UNIT_MATRIX SONIC_EXE");tests(exact<sadx::NinjaMath::SineTable>(argv[1]),exact<sadx::NinjaMatrix>(argv[2]),bytes(argv[3]));std::cout<<"Mesh registry: source sentinel capacities, reuse, actual ECoast triangle production and unknown boundaries passed.\n";return 0;}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
