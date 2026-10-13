#include "player/player_mesh_query.hpp"
#include <cstring>
#include <fstream>
#include <iostream>
#include <stdexcept>

namespace {
void require(bool condition,const char* message){if(!condition)throw std::runtime_error(message);}
template<class T>T exact(const char* path){T value;std::ifstream f(path,std::ios::binary);f.read(reinterpret_cast<char*>(&value),sizeof(value));require(f.gcount()==sizeof(value)&&f.peek()==std::char_traits<char>::eof(),"Exact source artifact required");return value;}
void tests(const sadx::NinjaMath::SineTable& sine,const sadx::NinjaMatrix& unit,const sadx::FloorAngleConstants& fc,
    const sadx::PlayerMeshQueryConstants& qc,const sadx::CoordinateAxisConstants& axis){
    sadx::PlayerContactWork contact;contact.radius_at_0=1;contact.center_at_4=sadx::NinjaVector{1,1,1};contact.displacement_at_10=sadx::NinjaVector{0,-2,0};contact.contact_count_at_28=23;
    sadx::PlayerSceneLandscape scene;sadx::PlayerMeshQueryState state;state.contact_count=17;
    sadx::PlayerMeshQueryRuntime runtime{{1,{}},{1,{}},{1,0,{}},10000};
    const auto run=[&](){
        sadx::NinjaMatrixStack stack(std::vector<sadx::NinjaMatrix>(4,unit),1,0);sadx::PlayerCoordinates coordinates(sine,unit,stack);sadx::X87Context fp(0x027f);
        const auto result=sadx::query_mesh_object_contacts(contact,scene,sadx::SceneResourceReference{1},state,qc,fc,axis,runtime,coordinates,fp);
        return std::pair{result,stack.depth()};
    };
    contact.radius_at_0.reset();auto [result,depth]=run();require(result.failure==sadx::PlayerMeshQueryFailure::unavailable_contact_field && *state.contact_count==17,"Unknown radius cannot reset source scratch");
    contact.radius_at_0=0;std::tie(result,depth)=run();require(static_cast<bool>(result)&&result.outcome==0&&*state.contact_count==17,"Source nonpositive radius returns0 before scratch reset");
    contact.radius_at_0=1;std::tie(result,depth)=run();require(result.failure==sadx::PlayerMeshQueryFailure::unavailable_group_selector&&*state.contact_count==0&&*contact.contact_count_at_28==23,"Unavailable scene selector retains reset scratch and prior contact outputs");
    scene.mesh_groups=sadx::SceneResourceReference{};std::tie(result,depth)=run();require(static_cast<bool>(result)&&result.outcome==0,"Observed null group list is the actual completed no-match path");
    scene.mesh_groups=sadx::SceneResourceReference{0};scene.groups[0]={sadx::SceneResourceReference{},sadx::SceneResourceReference{1},{},{},{},0,1};
    std::tie(result,depth)=run();require(result.failure==sadx::PlayerMeshQueryFailure::unavailable_selected_node,"Unknown selected resource cannot become an empty triangle");
    scene.selected_nodes[0]=0;std::tie(result,depth)=run();require(result.failure==sadx::PlayerMeshQueryFailure::unresolved_node,"Missing node owner remains a precise failure");
    scene.nodes[0]={};std::tie(result,depth)=run();require(result.failure==sadx::PlayerMeshQueryFailure::unavailable_triangle,"Bound data alone does not invent polygon vertices");
    scene.nodes[0].triangle_vertices=std::array<sadx::NinjaVector,3>{{{0,0,0},{20,0,0},{0,0,20}}};
    std::tie(result,depth)=run();require(result.failure==sadx::PlayerMeshQueryFailure::unavailable_material_flags && depth==2 && !contact.vector_at_3c,
        "Missing materialflags retains preceding pushed frame without fabricated normal output");
    scene.nodes[0].raw_at_3c=0;runtime.inverse_math.matherr_control.reset();
    std::tie(result,depth)=run();require(result.failure==sadx::PlayerMeshQueryFailure::normalization_unavailable&&result.normal_normalization&&result.normal_normalization->sqrt&&
        result.normal_normalization->sqrt->failure==sadx::CrtAsinFailure::unknown_matherr_policy&&depth==2,"Unknown CRT policy preserves full nested pending facts and frame");
    runtime.inverse_math.matherr_control=1;runtime.planar_angle.matherr_control.reset();
    std::tie(result,depth)=run();require(result.failure==sadx::PlayerMeshQueryFailure::atan2_unavailable&&result.atan2&&result.atan2->failure==sadx::CrtAtan2Failure::unknown_matherr_policy&&
        *state.contact_count==0&&*contact.contact_count_at_28==23,"Unknown two-argument CRT state must not manufacture a contact payload");
    runtime.planar_angle.matherr_control=1;std::tie(result,depth)=run();require(static_cast<bool>(result)&&result.outcome==1&&*contact.contact_count_at_28==1&&contact.vector_at_3c&&state.records[0].payload&&depth==1,
        "Actual finite triangle query completes through real math and source contact stores");
    runtime.sample_budget=0;std::tie(result,depth)=run();require(result.failure==sadx::PlayerMeshQueryFailure::sample_budget_exhausted&&!result.outcome&&depth==2,
        "Caller work budget is an explicit failure, never a no-contact success or rollback");
}
}
int main(int argc,char** argv){
    try{
        require(argc==6,"usage: mesh_query_test QUARTER_TABLE UNIT_MATRIX FLOOR_CONSTANTS_36 QUERY_CONSTANTS_32 AXIS_CONSTANTS_8");
        const auto floor=exact<std::array<std::uint8_t,36>>(argv[3]);sadx::FloorAngleConstants fc;
        std::memcpy(&fc.upper_cut,floor.data(),4);std::memcpy(&fc.lower_cut,floor.data()+4,4);std::memcpy(&fc.axis_epsilon,floor.data()+8,4);std::memcpy(&fc.angle_units,floor.data()+12,8);std::memcpy(&fc.positive_factor,floor.data()+20,8);std::memcpy(&fc.negative_factor,floor.data()+28,8);
        const auto query=exact<std::array<std::uint8_t,32>>(argv[4]);sadx::PlayerMeshQueryConstants qc;
        std::memcpy(&qc.half_radius,query.data(),4);std::memcpy(&qc.three,query.data()+4,4);std::memcpy(&qc.negative_one,query.data()+8,4);std::memcpy(&qc.triangle_expansion,query.data()+12,4);std::memcpy(&qc.angle_units,query.data()+16,8);std::memcpy(&qc.angle_divisor,query.data()+24,8);
        tests(exact<sadx::NinjaMath::SineTable>(argv[1]),exact<sadx::NinjaMatrix>(argv[2]),fc,qc,exact<sadx::CoordinateAxisConstants>(argv[5]));
        std::cout<<"Mesh query: actual triangle contacts, precise pending failures and source frame retention passed.\n";return 0;
    }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
