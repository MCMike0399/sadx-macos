#include "guest/player_scene_position_reference.hpp"
#include <cstring>
#include <fstream>
#include <iostream>
#include <stdexcept>
namespace {
void require(bool value,const char* message){if(!value)throw std::runtime_error(message);}
std::vector<std::uint8_t> read(const char* path){std::ifstream file(path,std::ios::binary);return {std::istreambuf_iterator<char>(file),{}};}
template<class T>T exact(const char* path){const auto bytes=read(path);require(bytes.size()==sizeof(T),"Exact source asset required");T result;std::memcpy(&result,bytes.data(),sizeof(result));return result;}
void tests(int argc,char** argv){
    require(argc==12,"usage: scene_position_test TRIG UNIT PARAMS SCENE124 ROTATION88 INIT52 RESPONSE52 ATTR40 FLOOR36 QUERY32 AXIS8");
    const auto sine=exact<sadx::NinjaMath::SineTable>(argv[1]);const auto unit=exact<sadx::NinjaMatrix>(argv[2]);const auto rows=read(argv[3]);require(rows.size()>=132,"Source parameter row");const auto parameters=sadx::decode_player_parameters(rows.data(),132);require(static_cast<bool>(parameters),"Parameter decoder");
    const auto cb=read(argv[4]);const auto constants=sadx::decode_scene_position_constants(cb.data(),cb.size());require(constants.has_value(),"Scene constants");
    const auto rb=exact<std::array<std::uint8_t,88>>(argv[5]);sadx::PlayerRotationConstants rotation;std::memcpy(rotation.float_bits.data(),rb.data(),64);std::memcpy(rotation.double_bits.data(),rb.data()+64,24);
    sadx::PlayerContactInitializationConstants initialization;const auto ib=exact<std::array<std::uint8_t,52>>(argv[6]);std::memcpy(initialization.bits.data(),ib.data(),52);
    const auto response_bytes=read(argv[7]),attribute_bytes=read(argv[8]);const auto response=sadx::decode_contact_response_constants(response_bytes.data(),response_bytes.size());const auto attribute=sadx::decode_contact_attribute_constants(attribute_bytes.data(),attribute_bytes.size());require(response&&attribute,"Source response/attribute constants");
    const auto fb=exact<std::array<std::uint8_t,36>>(argv[9]);sadx::FloorAngleConstants floor;std::memcpy(&floor.upper_cut,fb.data(),4);std::memcpy(&floor.lower_cut,fb.data()+4,4);std::memcpy(&floor.axis_epsilon,fb.data()+8,4);std::memcpy(&floor.angle_units,fb.data()+12,8);std::memcpy(&floor.positive_factor,fb.data()+20,8);std::memcpy(&floor.negative_factor,fb.data()+28,8);
    const auto qb=exact<std::array<std::uint8_t,32>>(argv[10]);sadx::PlayerMeshQueryConstants query;std::memcpy(&query.half_radius,qb.data(),4);std::memcpy(&query.three,qb.data()+4,4);std::memcpy(&query.negative_one,qb.data()+8,4);std::memcpy(&query.triangle_expansion,qb.data()+12,4);std::memcpy(&query.angle_units,qb.data()+16,8);std::memcpy(&query.angle_divisor,qb.data()+24,8);const auto axis=exact<sadx::CoordinateAxisConstants>(argv[11]);
    sadx::PlayerTaskState task{};task.orientation.flags=1;task.position=sadx::NinjaVector{1,2,3};task.action_at_0=0;
    sadx::PlayerMotionState world{};world.motion.speed={1,-1,2};sadx::PlayerPhysicsState player{};player.flags_at_10=7;player.prior_attributes=0;player.motion.speed={1,-1,2};player.floor_normal=sadx::NinjaVector{0,1,0};player.moving_task_at_6c=sadx::TaskHandle{};player.cached_land_object_at_74=sadx::SceneResourceReference{};player.no_control_time_at_a=0;
    sadx::PlayerContactWork contact;contact.center_at_4=sadx::NinjaVector{0,0,0};contact.direction_at_1c=sadx::NinjaVector{9,8,7};contact.angle_x_at_34=99;contact.angle_z_at_38=88;player.contact_work=contact;
    sadx::PlayerParameters live=*parameters.parameters;sadx::ContactParameterTable defaults{};defaults[0]={live.weight,live.air_resist_y,live.run_accel};sadx::PlayerMeshQueryState contacts;
    sadx::PlayerScenePositionGlobals globals;globals.special_surface_at_3b42e74=77;globals.motion_override_at_3b42e70=0;globals.event_task_at_3b2c578=sadx::TaskHandle{};
    sadx::PlayerMeshQueryRuntime math{{1,{}},{1,{}},{1,0,{}},1000};const std::optional<sadx::NinjaVector> gravity=sadx::NinjaVector{0,-1,0};const sadx::PlayerContactResponseRuntime response_math{math.inverse_math,gravity,&sine,{}};
    sadx::PlayerInputRuntime input;input.stage=1;input.normalized[0]=sadx::NormalizedPlayerInput{0,0};input.no_control_timer[0]=0;const sadx::ForcedInputAngleConversion forced;
    const sadx::SceneBroadphaseConstants broadphase{0,1};sadx::PlayerSceneLandscape scene;scene.dynamic_count=0;scene.static_enabled=0;scene.mesh_groups=sadx::SceneResourceReference{};
    sadx::PlayerScenePositionRuntime runtime{globals,contacts,math,response_math,input,forced,{nullptr,nullptr},nullptr,defaults,*attribute,*response,initialization,rotation,floor,query,broadphase,axis};
    const auto run=[&](bool alternate=false,bool motion_available=true){auto* motion=motion_available?&world:nullptr;sadx::NinjaMatrixStack stack(std::vector<sadx::NinjaMatrix>(8,unit),1,0);sadx::PlayerCoordinates coordinates(sine,unit,stack);sadx::X87Context fp(0x027f);auto result=alternate?sadx::position_player_in_scene_alternate(task,motion,player,live,scene,*constants,runtime,coordinates,fp):sadx::position_player_in_scene(task,motion,player,live,scene,*constants,runtime,coordinates,fp);return std::pair{result,stack.depth()};};
    player.flags_at_10.reset();auto [result,depth]=run();require(result.failure==sadx::ScenePositionFailure::unavailable_field&&task.orientation.flags==1&&player.contact_work->direction_at_1c->x==9,"Unknown prior flags stop before any source stores");
    player.flags_at_10=7;player.prior_attributes.reset();std::tie(result,depth)=run();require(result.source_boundary==0x44958d&&*player.flags_at_10==1&&task.orientation.flags==0&&player.contact_work->direction_at_1c->y==1&&*globals.special_surface_at_3b42e74==0,"Missing prior attributes retains preceding literal/contact/flag stores");
    player.prior_attributes=0;task.orientation.flags=0;std::tie(result,depth)=run(false,false);require(result.failure==sadx::ScenePositionFailure::null_motion&&player.contact_work->direction_at_1c&&depth==1,"Required null motion is an explicit source boundary");
    player.contact_work->center_at_4=sadx::NinjaVector{0,0,0};task.position=sadx::NinjaVector{1,2,3};std::tie(result,depth)=run();require(static_cast<bool>(result)&&result.outcome==0&&task.position->x==2&&task.position->y==1&&task.position->z==5&&!player.moving_task_at_6c->get()&&!player.cached_land_object_at_74->id&&depth==1,"Actual known-empty source landscape follows complete move/attribute/null-store tail");
    task.position=sadx::NinjaVector{1,2,3};std::tie(result,depth)=run(true);require(static_cast<bool>(result)&&result.outcome==0&&depth==1,"Alternate full no-contact body restores actual matrix frame");
}
}
int main(int argc,char** argv){try{tests(argc,argv);std::cout<<"Scene position: exact early stores, unavailable dependencies and both complete no-contact bodies passed.\n";return 0;}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
