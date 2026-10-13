#include "player/player_mesh_registry.hpp"
#include "player/player_contact_response.hpp"
#include "player/player_rotation.hpp"
#include <cstring>
#include <fstream>
#include <iostream>
#include <stdexcept>

namespace {
void require(bool condition,const char* text){if(!condition)throw std::runtime_error(text);}
template<class T>T exact(const char* path){T value;std::ifstream file(path,std::ios::binary);file.read(reinterpret_cast<char*>(&value),sizeof(value));require(file.gcount()==sizeof(value)&&file.peek()==std::char_traits<char>::eof(),"Exact source asset required");return value;}
std::vector<std::uint8_t> bytes(const char* path){std::ifstream file(path,std::ios::binary);return {std::istreambuf_iterator<char>(file),{}};}
void test(int argc,char** argv){
    require(argc==9,"usage: mesh_registry_contacts TRIG UNIT SONIC_EXE FLOOR_36 QUERY_32 AXIS_8 SCENE_12 RESPONSE_52");
    const auto trig=exact<sadx::NinjaMath::SineTable>(argv[1]);const auto unit=exact<sadx::NinjaMatrix>(argv[2]);const auto image=bytes(argv[3]);
    const auto floor=exact<std::array<std::uint8_t,36>>(argv[4]);sadx::FloorAngleConstants fc;std::memcpy(&fc.upper_cut,floor.data(),4);std::memcpy(&fc.lower_cut,floor.data()+4,4);std::memcpy(&fc.axis_epsilon,floor.data()+8,4);std::memcpy(&fc.angle_units,floor.data()+12,8);std::memcpy(&fc.positive_factor,floor.data()+20,8);std::memcpy(&fc.negative_factor,floor.data()+28,8);
    const auto query=exact<std::array<std::uint8_t,32>>(argv[5]);sadx::PlayerMeshQueryConstants qc;std::memcpy(&qc.half_radius,query.data(),4);std::memcpy(&qc.three,query.data()+4,4);std::memcpy(&qc.negative_one,query.data()+8,4);std::memcpy(&qc.triangle_expansion,query.data()+12,4);std::memcpy(&qc.angle_units,query.data()+16,8);std::memcpy(&qc.angle_divisor,query.data()+24,8);
    const auto axis=exact<sadx::CoordinateAxisConstants>(argv[6]);const auto scene_constants=exact<std::array<float,3>>(argv[7]);
    const auto response_bytes=exact<std::array<std::uint8_t,52>>(argv[8]);
    const auto response_constants=sadx::decode_contact_response_constants(response_bytes.data(),response_bytes.size());
    require(response_constants.has_value(),"Exact source contact-response constants");
    auto decoded=sadx::decode_stage_scene_landtable(image.data(),image.size(),1,0);require(static_cast<bool>(decoded),"Source LANDTABLE decode");auto& land=*decoded.table;land.source_count=1; // explicit real first-record resource view
    sadx::PlayerSceneLandscape scene;scene.objects=land.objects;scene.models=land.models;scene.vector_arrays=land.vector_arrays;scene.mesh_set_arrays=land.mesh_set_arrays;scene.index_arrays=land.index_arrays;scene.material_arrays=land.material_arrays;scene.static_enabled=1;scene.landtable=sadx::SceneResourceReference{0};
    sadx::MeshRegistryPools pools;for(std::size_t i=0;i<256;++i)pools.groups[i]=i;for(std::size_t i=0;i<16384;++i)pools.nodes[i]=i;
    sadx::PlayerMeshRegistryState registry;sadx::PlayerMeshQueryState contacts;require(static_cast<bool>(sadx::initialize_mesh_registry(scene,registry,contacts,pools)),"Source initializer");registry.force_refresh=1;
    sadx::NinjaMatrixStack producer_stack(std::vector<sadx::NinjaMatrix>(8,unit),1,0);sadx::PlayerCoordinates producer_coordinates(trig,unit,producer_stack);sadx::X87Context fp(0x027f);sadx::CrtFabsRuntime absolute{1,0,{}};
    sadx::SceneRegistrationRuntime registration;registration.registered_count=0;
    const auto produced=sadx::refresh_mesh_registry(scene,registry,{land,registration,{scene_constants[2]},absolute},producer_coordinates,fp);
    require(static_cast<bool>(produced)&&scene.mesh_groups->id&&scene.groups.at(*scene.mesh_groups->id).head->id,"Actual refresh produces a registered group with polygon data");
    const auto object=*scene.groups.at(*scene.mesh_groups->id).object;auto link=scene.groups.at(*scene.mesh_groups->id).head;
    std::size_t attempts=0;bool hit=false;
    while(link&&link->id&&attempts++<128){
        const auto& node=scene.nodes.at(*link->id);const auto triangle=*node.triangle_vertices;sadx::NinjaVector normal;
        sadx::mesh_triangle_normal(triangle[0],triangle[1],triangle[2],normal,fp);const auto normalized=sadx::normalize_mesh_vector_zero(normal,axis,{1,{}},fp);require(static_cast<bool>(normalized),"Actual triangle normal owner");
        sadx::PlayerContactWork contact;contact.radius_at_0=1;
        const auto scaled=[&](float n,float factor){return fp.to_float(fp.multiply_float_memory(fp.from_float(n),factor));};
        const auto above=[&](float p,float n){return fp.to_float(fp.add_float_memory(fp.from_float(p),scaled(n,5)));};
        contact.center_at_4=sadx::NinjaVector{above(node.center->x,normal.x),above(node.center->y,normal.y),above(node.center->z,normal.z)};
        contact.displacement_at_10=sadx::NinjaVector{scaled(normal.x,-10),scaled(normal.y,-10),scaled(normal.z,-10)};
        sadx::NinjaMatrixStack query_stack(std::vector<sadx::NinjaMatrix>(8,unit),1,0);sadx::PlayerCoordinates coordinates(trig,unit,query_stack);
        const auto selected=sadx::query_player_scene_broadphase(scene,contact,{scene_constants[0],scene_constants[1]},coordinates,fp);require(static_cast<bool>(selected),"Actual broadphase uses producer-owned groups/nodes");
        sadx::PlayerMeshQueryRuntime runtime{{1,{}},{1,{}},{1,0,{}},10000};
        const auto result=sadx::query_mesh_object_contacts(contact,scene,object,contacts,qc,fc,axis,runtime,coordinates,fp);
        require(static_cast<bool>(result),"Actual453820 receives generated polygons without a callback or selected-node seed");
        if(result.outcome&&*result.outcome){
            require(contact.contact_count_at_28&&*contact.contact_count_at_28&&contacts.records[0].node_snapshot&&contacts.records[0].payload,"Source contact outputs retain generated-node identity and payload");
            // Explicit contact frame derived from actual generated geometry.
            // The response consumes the query's produced payloads unchanged.
            contact.direction_at_1c=normal;
            const std::optional<sadx::NinjaVector> gravity=sadx::NinjaVector{0,-1,0};
            const sadx::CrtAsinRuntime inverse{1,{}};
            const sadx::PlayerContactResponseRuntime response_runtime{inverse,gravity,&trig,{}};
            const sadx::PlayerContactResponseActors unused_actors{nullptr,nullptr,nullptr,nullptr,{}};
            const auto before=sadx::rotation_vector_length(*contact.displacement_at_10,fp);
            const auto response=sadx::respond_player_contacts(sadx::PlayerContactResponseEntry::at_43d180,
                contact,contacts,unused_actors,*response_constants,response_runtime,fp);
            require(static_cast<bool>(response),"Actual43d180 consumes actual453820 response records");
            if(*response.outcome==1){
                require(response.candidates>0 && response.selected>=0 &&
                    fp.compare(sadx::rotation_vector_length(*contact.displacement_at_10,fp),before)==sadx::X87Comparison::less,
                    "Generated contacts shorten the crossing displacement through the native response");
                hit=true;break;
            }
        }
        link=node.next;
    }
    require(hit,"Real ECoast generated mesh must produce and resolve a contact through native broadphase/query/response");
}
}
int main(int argc,char** argv){try{test(argc,argv);std::cout<<"Mesh composition: actual ECoast registry generation -> broadphase ->453820 contact ->43d180 response passed.\n";return 0;}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
