#include "player/player_acceleration_air.hpp"
#include <cstring>
#include <fstream>
#include <iostream>
#include <stdexcept>

namespace {
void require(bool value,const char* message) { if(!value) throw std::runtime_error(message); }
template<class T> T read(const char* path,bool exact=true) {
    T value{};std::ifstream file(path,std::ios::binary);file.read(reinterpret_cast<char*>(&value),sizeof(value));
    require(file.gcount()==static_cast<std::streamsize>(sizeof(value)) &&
        (!exact || file.peek()==std::char_traits<char>::eof()),"source asset missing or wrong size");return value;
}
}
int main(int argc,char** argv) {
    try {
        require(argc==5,"usage: player-air-test PARAMETER_TABLE QUARTER_TABLE UNIT_MATRIX ROTATION_CONSTANTS");
        const auto bytes=read<std::array<std::uint8_t,132>>(argv[1],false);
        const auto decoded=sadx::decode_player_parameters(bytes.data(),bytes.size());
        require(static_cast<bool>(decoded),"source Sonic parameter decode failed");
        const auto table=read<sadx::NinjaMath::SineTable>(argv[2]);
        const auto unit=read<sadx::NinjaMatrix>(argv[3]);
        const auto rotation_bytes=read<std::array<std::uint8_t,88>>(argv[4]);
        sadx::PlayerRotationConstants rotation_constants;
        std::memcpy(rotation_constants.float_bits.data(),rotation_bytes.data(),64);
        std::memcpy(rotation_constants.double_bits.data(),rotation_bytes.data()+64,24);
        const sadx::AirAccelerationConstants constants{rotation_constants.single(0x7dcb2c)};
        sadx::NinjaMatrixStack stack({unit,unit,unit},1,0);
        sadx::PlayerCoordinates coordinates(table,unit,stack);
        sadx::X87Context fp(0x027f); // Selected experiment, not original startup state.
        sadx::PlayerTaskState task{{0,{0,0,0}},0,0,{},{}};
        sadx::PlayerMotionState world{{{0,-1,0},{0,0,0}},12345,7,8,1};
        sadx::PlayerPhysicsState player{{{11,12,13},{0,0,0}},1,{0,0,0},{},0,0,{}};
        sadx::PlayerAccelerationRuntime runtime;
        sadx::AirAccelerationCallees callees;
        auto result=sadx::get_player_acceleration_air(task,world,player,*decoded.parameters,constants,runtime,callees,fp);
        require(!result && result.acceleration.failure==sadx::PlayerAccelerationFailure::unavailable_gravity &&
            !result.acceleration.temporary_acceleration && player.motion.acceleration.x==11,
            "unknown gravity cannot invent temporary zero facts or update player acceleration");
        task.orientation.flags=0x4000;
        result=sadx::get_player_acceleration_air(task,world,player,*decoded.parameters,constants,runtime,callees,fp);
        require(result.acceleration.failure==sadx::PlayerAccelerationFailure::missing_callee &&
            result.acceleration.callee_address==0x447b90,"event priority must precede gravity reads");
        task.orientation.flags=0;
        runtime.world_gravity=sadx::NinjaVector{0,-1,0};
        runtime.input.stage=1;runtime.input.normalized[0]=sadx::NormalizedPlayerInput{0,0.0F};
        runtime.input.no_control_timer[0]=0;
        callees.world_to_player=[&](const auto& orientation,auto& vector,auto& context) {
            return coordinates.world_to_player(orientation,vector,context);
        };
        result=sadx::get_player_acceleration_air(task,world,player,*decoded.parameters,constants,runtime,callees,fp);
        require(static_cast<bool>(result) && world.target_yaw==0 && player.motion.acceleration.x==0 &&
            player.motion.acceleration.y==-decoded.parameters->weight && player.motion.acceleration.z==0,
            "inactive path uses real gravity coordinates and source acceleration stores");
        runtime.input.normalized[0]=sadx::NormalizedPlayerInput{0,0.5F};
        sadx::PlayerRotationRuntime rotation_runtime;
        rotation_runtime.input=runtime.input;
        rotation_runtime.passed_player_no_control_timer=0;
        callees.yaw_regular=[&](auto& selected_task,auto& selected_world,auto& selected_player,
                                std::int32_t target,auto& context) {
            return sadx::rotate_player_regular(selected_task,selected_world,selected_player,target,
                rotation_constants,rotation_runtime,coordinates,context);
        };
        result=sadx::get_player_acceleration_air(task,world,player,*decoded.parameters,constants,runtime,callees,fp);
        require(static_cast<bool>(result) && result.rotation && static_cast<bool>(*result.rotation) &&
            player.motion.acceleration.x==decoded.parameters->air_accel && stack.depth()==1,
            "active descending path doubles control gain and executes real regular yaw");
        const auto old_acceleration=player.motion.acceleration;
        runtime.input.normalized[0]=sadx::NormalizedPlayerInput{16384,0.5F};
        callees.yaw_regular={};
        result=sadx::get_player_acceleration_air(task,world,player,*decoded.parameters,constants,runtime,callees,fp);
        require(!result && world.target_yaw==16384 && player.motion.acceleration.x==old_acceleration.x &&
            result.acceleration.temporary_acceleration,"missing yaw retains prior target and temporary stores");
        task.orientation.flags=0x4000;
        callees.event_acceleration=[](auto&,auto&,auto&,const auto&,auto&) {
            return sadx::PlayerAccelerationResult{sadx::PlayerAccelerationFailure::unavailable_event_vector,0x447bee};
        };
        result=sadx::get_player_acceleration_air(task,world,player,*decoded.parameters,constants,runtime,callees,fp);
        require(!result && result.acceleration.source_boundary==0x447bee &&
            result.acceleration.failure==sadx::PlayerAccelerationFailure::unavailable_event_vector,
            "delegated event error facts are retained, not reduced to a success flag");
        std::cout<<"Air acceleration: shared owners, gravity/input/real yaw, source control branches and partial failures passed.\n";
    } catch(const std::exception& error) { std::cerr<<error.what()<<'\n';return 1; }
}
