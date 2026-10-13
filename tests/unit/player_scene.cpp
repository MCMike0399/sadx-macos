#include "player/player_scene_registration.hpp"
#include <cstring>
#include <fstream>
#include <iostream>
#include <stdexcept>

namespace {
void require(bool condition,const char* message){if(!condition)throw std::runtime_error(message);}
template<class T>T exact(const char* path){T value;std::ifstream f(path,std::ios::binary);f.read(reinterpret_cast<char*>(&value),sizeof(value));require(f.gcount()==sizeof(value)&&f.peek()==std::char_traits<char>::eof(),"Exact source numeric artifact required");return value;}
float number(std::uint32_t w){float f;std::memcpy(&f,&w,4);return f;}
void tests(const std::vector<std::uint8_t>& bytes,const sadx::NinjaMath::SineTable& sine,const sadx::NinjaMatrix& unit,
    const std::array<std::uint32_t,3>& words){
    const auto decoded=sadx::decode_stage_scene_landtable(bytes.data(),bytes.size(),1,0);require(static_cast<bool>(decoded)&&decoded.table,"Actual source stage1/act0 must decode");
    const auto& table=*decoded.table;require(table.table_location.source_virtual_address==0xe99cb8 && table.source_count==480 && table.entries.size()==480,
        "Actual ECoast LANDTABLE identity/count must survive GeoLists lookup");
    require(sadx::decode_stage_scene_landtable(bytes.data(),bytes.size(),1,3).failure==sadx::SceneDecodeFailure::null_landtable,
        "Observed null stage table cannot become an empty level");
    require(sadx::decode_scene_landtable(bytes.data(),64,0xe99cb8).failure==sadx::SceneDecodeFailure::invalid_pe_image,
        "Truncated PE boundary must remain a tagged failure");
    sadx::PlayerSceneLandscape scene;scene.objects=table.objects;scene.models=table.models;scene.static_enabled=1;scene.landtable=sadx::SceneResourceReference{0};scene.mesh_groups=sadx::SceneResourceReference{};
    scene.vector_arrays=table.vector_arrays;scene.mesh_set_arrays=table.mesh_set_arrays;scene.index_arrays=table.index_arrays;
    scene.material_arrays=table.material_arrays;
    sadx::NinjaMatrixStack stack(std::vector<sadx::NinjaMatrix>(3,unit),1,0);sadx::PlayerCoordinates coordinates(sine,unit,stack);sadx::X87Context fp(0x027f);
    const sadx::SceneRegistrationConstants registration{number(words[2])};const sadx::SceneBroadphaseConstants broad{number(words[0]),number(words[1])};
    sadx::SceneRegistrationRuntime runtime;scene.candidate_count=17;
    auto result=sadx::register_player_scene_landscape(scene,table,runtime,registration,coordinates,fp);
    require(result.failure==sadx::SceneQueryFailure::unavailable_field && scene.candidate_count==17,"Unknown camera selector cannot initialize registries");
    runtime.camera=sadx::SceneTaskReadView{nullptr};result=sadx::register_player_scene_landscape(scene,table,runtime,registration,coordinates,fp);
    require(static_cast<bool>(result)&&scene.candidate_count==17 && !scene.dynamic_count,"Observed null camera retains prior registry bytes/counts");
    sadx::PlayerTaskState camera{{0,{0,0,0}},0,0,{},table.entries[0].center};runtime.camera=sadx::SceneTaskReadView{&camera};
    runtime.first_player=sadx::SceneTaskReadView{nullptr};runtime.second_player=sadx::SceneTaskReadView{nullptr};runtime.registered_count=0;runtime.view_distance=1000;
    result=sadx::register_player_scene_landscape(scene,table,runtime,registration,coordinates,fp);
    require(static_cast<bool>(result)&&scene.static_count && *scene.static_count>0 && *scene.dynamic_count==0 && stack.depth()==1,
        "Real records must produce source-capped static registration through live view ownership");
    sadx::PlayerContactWork contact;contact.center_at_4=table.entries[0].center;contact.radius_at_0=1;contact.displacement_at_10=sadx::NinjaVector{0,0,0};
    result=sadx::query_player_scene_broadphase(scene,contact,broad,coordinates,fp);
    require(static_cast<bool>(result)&&scene.candidate_count>0 && scene.selected_count==0,"Actual static record query completes with an explicit null mesh registry");
    camera.position=sadx::NinjaVector{1e9F,1e9F,1e9F};runtime.view_distance=10;
    result=sadx::register_player_scene_landscape(scene,table,runtime,registration,coordinates,fp);
    require(static_cast<bool>(result)&&*scene.static_count==0,"Borrowed camera views must observe new source pose rather than stale snapshots");
    // PopEx permits source depth0; subsequent PushMatrix is an unavailable
    // ownership boundary. Preceding candidate resets must remain committed.
    stack.pop_ex();scene.candidate_count=17;
    result=sadx::collect_scene_land_candidates(scene,{0,0,0},1,broad,coordinates,fp);
    require(result.failure==sadx::SceneQueryFailure::invalid_matrix_owner && result.source_boundary==0x43ace6 &&
        result.coordinate_failure==sadx::CoordinateFailure::invalid_matrix_stack && scene.candidate_count==0,
        "Invalid matrix ownership needs a source boundary after the exact reset");
    sadx::NinjaMatrixStack cap_stack(std::vector<sadx::NinjaMatrix>(3,unit),1,0);sadx::PlayerCoordinates cap_coordinates(sine,unit,cap_stack);
    camera.position=table.entries[0].center;camera.orientation.flags=0x100;runtime.view_distance=1e6F;runtime.registered_count=128;
    const sadx::SceneRegisteredLandEntry entry{{0,table.entries[0].object,sadx::SceneResourceReference{0}},sadx::SceneTaskReadView{&camera}};
    runtime.registered_entries.assign(128,entry);
    result=sadx::register_player_scene_landscape(scene,table,runtime,registration,cap_coordinates,fp);
    require(static_cast<bool>(result)&&*scene.dynamic_count==128 && scene.static_count && cap_stack.depth()==1,
        "Diagnostic at128 must continue source pop/static registration rather than abort geometry work");
}
}
int main(int argc,char** argv){
    try{
        require(argc==5,"usage: player_scene_test SOURCE_EXE QUARTER_TABLE UNIT_MATRIX SCENE_CONSTANTS_12");std::ifstream f(argv[1],std::ios::binary);
        const std::vector<std::uint8_t> bytes{std::istreambuf_iterator<char>(f),std::istreambuf_iterator<char>()};require(!bytes.empty(),"Source artifact required");
        tests(bytes,exact<sadx::NinjaMath::SineTable>(argv[2]),exact<sadx::NinjaMatrix>(argv[3]),exact<std::array<std::uint32_t,3>>(argv[4]));
        std::cout<<"Scene: real ECoast decode/registration/query, live readviews and partial resource failures passed.\n";return 0;
    }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
