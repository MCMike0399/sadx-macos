#include "player/player_acceleration_alternate.hpp"
#include "player/player_acceleration_alternate_math.hpp"
#include "player/player_heading.hpp"

#include <cstring>
#include <fstream>
#include <iostream>
#include <stdexcept>

namespace {
void require(bool condition,const char* text){if(!condition)throw std::runtime_error(text);}
template<class T>T exact(const char* path){T result;std::ifstream file(path,std::ios::binary);file.read(reinterpret_cast<char*>(&result),sizeof(result));require(file.gcount()==sizeof(result)&&file.peek()==std::char_traits<char>::eof(),"Exact source artifact required");return result;}
bool same(const sadx::NinjaVector& a,const sadx::NinjaVector& b){return std::memcmp(&a,&b,sizeof(a))==0;}
void tests(const sadx::PlayerParameters& parameters,const sadx::GroundAccelerationConstants& constants,
    const sadx::EventAccelerationConstants& event_constants,const sadx::CoordinateAxisConstants& axis,
    const sadx::ProjectionAngleConstants& projection,sadx::PlayerCoordinates& coordinates) {
    sadx::PlayerTaskState task{{0x4000,{0,0,0}},0,0,{}, {}};
    sadx::PlayerMotionState world{{{0,0,0},{0,0,0}},0x1234,123,456,1};
    sadx::PlayerPhysicsState player{{{11,22,33},{3,0,1}},1,{0,0,0},{},0,0,{}};
    sadx::PlayerAccelerationRuntime runtime;runtime.world_gravity=sadx::NinjaVector{0,-1,0};
    runtime.input.stage=1;runtime.input.normalized[0]=sadx::NormalizedPlayerInput{0x4000,1};runtime.input.no_control_timer[0]=0;
    sadx::GroundAccelerationCallees callees;sadx::X87Context fp(0x027f);
    const auto initial_acceleration=player.motion.acceleration;
    const auto event_ground=[&](){return sadx::get_player_acceleration_event_ground(task,world,player,parameters,constants,event_constants,runtime,callees,fp);};
    auto result=event_ground();require(result.failure==sadx::PlayerAccelerationFailure::unavailable_event_mode &&
        result.temporary_acceleration && same(player.motion.acceleration,initial_acceleration),"Unknown event owner must stop without invented mode");
    task.event_move=sadx::PlayerEventMoveData{2,{}, {}, {}};
    result=event_ground();require(result.failure==sadx::PlayerAccelerationFailure::unavailable_event_vector,"Mode2 requires its actual vector");
    task.event_move->vector_at_28=sadx::NinjaVector{0,0,0};
    result=event_ground();require(result.failure==sadx::PlayerAccelerationFailure::unavailable_task_position,"Mode2 must not substitute an origin");
    task.position=sadx::NinjaVector{0,0,0};
    result=event_ground();require(result.failure==sadx::PlayerAccelerationFailure::missing_world_to_player &&
        *task.event_move->mode==1 && player.motion.speed.x==0 && player.motion.speed.y==0 && player.motion.speed.z==0 &&
        same(player.motion.acceleration,initial_acceleration),"Target arrival stores survive the following unresolved transform");
    task.event_move->mode=0;task.orientation.flags=0x4000;
    result=event_ground();require(result.failure==sadx::PlayerAccelerationFailure::missing_world_to_player &&
        (task.orientation.flags&0x4000)==0,"Out-of-range event mode clears exactly the observed task flag");
    task.event_move->mode=2;task.event_move->vector_at_28=sadx::NinjaVector{2,0,0};task.orientation.flags=0x4000;
    world.target_yaw=0x1234;
    result=event_ground();require(result.failure==sadx::PlayerAccelerationFailure::unavailable_event_target_speed &&
        world.target_yaw==0,"Direction store must precede unknown target speed");
    task.event_move->target_speed=0;
    result=event_ground();require(result.failure==sadx::PlayerAccelerationFailure::unavailable_event_acceleration_step,
        "Ground mode2 requires source step even when target speed falls back");
    callees.world_to_player=[&](const auto& orientation,auto& v,auto& context){return coordinates.world_to_player(orientation,v,context);};
    callees.yaw_fast=[&](auto& t,auto&,auto& p,const auto&,std::int32_t yaw,auto& context){
        const auto heading=sadx::turn_player_heading(t.orientation,p.motion,yaw,coordinates,context);sadx::PlayerAccelerationResult result;
        if(!heading){result.failure=sadx::PlayerAccelerationFailure::coordinate_unavailable;result.source_boundary=heading.source_boundary;result.coordinate_failure=heading.coordinate.failure;result.temporary_acceleration=heading.temporary_vector;}
        return result;};
    const auto air=sadx::get_player_acceleration_event_air(task,world,player,parameters,constants,event_constants,runtime,callees,fp);
    require(static_cast<bool>(air) && !task.event_move->acceleration_step,"Air mode2 must lazily ignore ground-only step");
    task.event_move->acceleration_step=0;
    result=event_ground();require(static_cast<bool>(result),"Actual source event-ground owner closure must complete");
    // Active event GetAnalog leaves query outputs untouched; the independent
    // surface entry must retain that uncertainty instead of consuming locals.
    task.event_move->mode=1;task.orientation.flags=0x4000;
    result=sadx::get_player_acceleration_attribute_4000(task,world,player,parameters,constants,runtime,callees,fp);
    require(result.failure==sadx::PlayerAccelerationFailure::input_outputs_unavailable && result.source_boundary==0x44716a,
        "Output-write provenance must reject unobserved native scratch values");
    task.orientation.flags=0;task.event_move.reset();player.floor_normal.reset();
    sadx::AlternateSurfaceRuntime surface;
    const auto surface_run=[&](const sadx::CrtAsinRuntime& crt){return sadx::get_player_acceleration_attribute_20000(task,world,player,
        parameters,constants,event_constants,runtime,surface,callees,coordinates,axis,projection,crt,fp);};
    result=surface_run({{}, {}});require(result.failure==sadx::PlayerAccelerationFailure::unavailable_floor_normal,
        "Surface attraction must not borrow a gravity/floor default");
    player.floor_normal=sadx::NinjaVector{0,1,0};player.motion.speed={3,0,1};runtime.input.normalized[0]=sadx::NormalizedPlayerInput{0,1};
    result=surface_run({{}, {}});require(result.failure==sadx::PlayerAccelerationFailure::unavailable_raw_axes,
        "Normalized analog angle must not replace source raw stick fields");
    surface.raw_axes[0]=std::array<std::int16_t,2>{1,1};
    result=surface_run({{}, {}});require(result.failure==sadx::PlayerAccelerationFailure::unavailable_camera,
        "Raw direction conversion must not manufacture camera orientation");
    surface.camera_pitch_yaw=std::array<std::int32_t,2>{0,0};
    result=surface_run({{}, {}});require(result.failure==sadx::PlayerAccelerationFailure::projection_unavailable &&
        result.projection && result.projection->crt_failure==sadx::CrtAsinFailure::unknown_matherr_policy &&
        result.projection->pending_value && result.floor_projection && result.floor_projection->forward_direction,
        "Unavailable CRT policy must preserve computed projection and pending value facts");
    result=surface_run({1, {}});require(result.failure==sadx::PlayerAccelerationFailure::missing_callee &&
        result.callee_address==0x443e60 && result.projection && result.floor_projection,
        "Successful projection facts survive missing blended-yaw owner");
    callees.yaw_blended=[](auto&,auto&,auto&,const auto&,std::int32_t,auto&){
        sadx::PlayerAccelerationResult child;
        child.failure=sadx::PlayerAccelerationFailure::coordinate_unavailable;
        child.source_boundary=0x443f53;
        child.temporary_acceleration=sadx::NinjaVector{123,456,789};
        child.rotation=sadx::PlayerRotationResult{sadx::PlayerRotationFailure::coordinate_unavailable,0x443f53};
        return child;
    };
    result=surface_run({1, {}});
    require(result.failure==sadx::PlayerAccelerationFailure::callee_unavailable &&
        result.callee_failure==sadx::PlayerAccelerationFailure::coordinate_unavailable &&
        result.source_boundary==0x44b52f && result.nested_source_boundary==0x443f53 &&
        result.temporary_acceleration && result.temporary_acceleration->x==123 &&
        result.rotation && result.rotation->source_boundary==0x443f53 && result.projection && result.floor_projection,
        "Caller boundaries must retain the child failure, rotation, temporary and computed projection facts");
    sadx::NinjaVector v{1,2,3};sadx::event_vector_subtract(v,v,fp);require(v.x==0&&v.y==0&&v.z==0,"Source subtraction permits aliasing");
    v={1,2,3};sadx::event_vector_add(v,v,fp);require(v.x==2&&v.y==4&&v.z==6,"Source addition permits aliasing");
}
}
int main(int argc,char** argv){
    try {
        require(argc==8,"usage: alternate_acceleration_test PARAMETER_BYTES GROUND_CONSTANTS EVENT_CONSTANTS_20 AXIS_CONSTANTS_8 PROJECTION_CONSTANTS_20 QUARTER_TABLE UNIT_MATRIX");
        std::array<std::uint8_t,132> parameter_bytes;std::ifstream params(argv[1],std::ios::binary);params.read(reinterpret_cast<char*>(parameter_bytes.data()),132);
        require(params.gcount()==132,"First source parameter row required");const auto decoded=sadx::decode_player_parameters(parameter_bytes.data(),132);require(static_cast<bool>(decoded),"Parameters failed");
        sadx::GroundAccelerationConstants constants{exact<std::array<std::uint32_t,24>>(argv[2])};
        const auto event_bytes=exact<std::array<std::uint8_t,20>>(argv[3]);sadx::EventAccelerationConstants ec;
        std::memcpy(&ec.target_stop_distance,event_bytes.data(),4);std::memcpy(&ec.angle_units,event_bytes.data()+4,8);std::memcpy(&ec.positive_factor,event_bytes.data()+12,8);
        const auto axis=exact<sadx::CoordinateAxisConstants>(argv[4]);
        const auto projection_bytes=exact<std::array<std::uint8_t,20>>(argv[5]);sadx::ProjectionAngleConstants pc;
        std::memcpy(&pc.negative_one,projection_bytes.data(),4);std::memcpy(&pc.first_factor,projection_bytes.data()+4,8);std::memcpy(&pc.second_factor,projection_bytes.data()+12,8);
        const auto table=exact<sadx::NinjaMath::SineTable>(argv[6]);const auto unit=exact<sadx::NinjaMatrix>(argv[7]);
        sadx::NinjaMatrixStack stack(std::vector<sadx::NinjaMatrix>(3,unit),1,0);sadx::PlayerCoordinates coordinates(table,unit,stack);
        tests(*decoded.parameters,constants,ec,axis,pc,coordinates);
        std::cout<<"Alternate acceleration: shared event mutations, lazy state, actual heading, query provenance and projection/CRT failure facts passed.\n";
        return 0;
    }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
