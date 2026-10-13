#include "player/player_movement.hpp"
#include <array>
#include <cmath>
#include <fstream>
#include <iostream>
#include <stdexcept>

static void require(bool value,const char* message) { if(!value) throw std::runtime_error(message); }
template<class T> static T read(const char* path) {
    T value{};
    std::ifstream stream(path,std::ios::binary);
    stream.read(reinterpret_cast<char*>(&value),sizeof(value));
    require(stream.gcount()==static_cast<std::streamsize>(sizeof(value)),"source asset missing or truncated");
    return value;
}

int main(int argc,char** argv) {
    try {
        require(argc==4,"usage: player-movement-test TRIG_TABLE UNIT_MATRIX PARAMETER_TABLE");
        const auto table=read<sadx::NinjaMath::SineTable>(argv[1]);
        const auto unit=read<sadx::NinjaMatrix>(argv[2]);
        const auto bytes=read<std::array<std::uint8_t,132>>(argv[3]);
        const auto parsed=sadx::decode_player_parameters(bytes.data(),bytes.size());
        require(static_cast<bool>(parsed),"source Sonic parameter record rejected");
        sadx::NinjaMatrixStack stack(std::vector<sadx::NinjaMatrix>(6,unit),1,0);
        sadx::PlayerCoordinates coordinates(table,unit,stack);
        sadx::X87Context fp(0x027f); // Explicit experimental profile, not a game default.
        sadx::PlayerTaskState task{{0,{0,0,0}},0,0,{}};
        sadx::PlayerInputRuntime input;
        input.stage=1; input.act=0;
        input.normalized[0]=sadx::NormalizedPlayerInput{{},1.0F};
        input.no_control_timer[0]=0;
        const sadx::FloorAngleConstants constants{0.9999600052833557F,-0.9999600052833557F,
            0.0020000000949949026F,65536.0,0.15915497620314795,-0.15915497620314795};
        const sadx::CrtAsinRuntime math{1,{}}; // Explicit CRT bypass profile for this fixture.
        sadx::MotionWork world{{4,5,6},{7,8,9}};
        sadx::PlayerPhysicsState player{{{0.1F,0,0},{1,0,0}},0.8F,{0,0,0},sadx::NinjaVector{0,1,0},0,0};
        auto result=sadx::run_player_movement(task,input,world,player,*parsed.parameters,coordinates,constants,math,fp);
        require(static_cast<bool>(result) && result.input && result.input->active==true &&
                result.coordinates && static_cast<bool>(*result.coordinates),
                "actual recovered input and task transforms were not bound");
        require(world.speed.x==1.1F && world.speed.y==0 && stack.depth()==1,
                "task path velocity or real stack restoration incorrect");
        player.attributes=0x4000; player.motion.speed={1,0,0};
        result=sadx::run_player_movement(task,input,world,player,*parsed.parameters,coordinates,constants,math,fp);
        require(static_cast<bool>(result) && result.floor_angles && result.floor_angles->angles &&
                result.floor_angles->angles->inclination==0 && result.floor_angles->angles->roll==0 &&
                world.speed.x==1.1F && stack.depth()==1,
                "real floor-angle and floor-coordinate owners did not execute");
        input.normalized[0].reset();
        player.attributes=0; player.motion.speed={1,0,0};
        const auto old_world=world.speed;
        result=sadx::run_player_movement(task,input,world,player,*parsed.parameters,coordinates,constants,math,fp);
        require(!result && result.input && result.input->failure==sadx::PlayerInputFailure::unavailable_normalized_input,
                "missing input producer replaced with zero/false input");
        require(player.motion.speed.x==1.1F && world.speed.x==old_world.x,
                "source partial stores lost on unavailable input boundary");
        input.normalized[0]=sadx::NormalizedPlayerInput{{},1.0F};
        player.attributes=0x4000; player.motion.speed={1,0,0};
        const sadx::CrtAsinRuntime unknown{{},{}};
        result=sadx::run_player_movement(task,input,world,player,*parsed.parameters,coordinates,constants,unknown,fp);
        require(!result && result.floor_angles &&
                result.floor_angles->failure==sadx::CrtAsinFailure::unknown_matherr_policy,
                "unavailable CRT policy lost behind coordinate failure");
        std::cout<<"Recovered movement composition: real input query, task/floor transforms, stack ownership and retained failure facts passed.\n"
                    "Normalized input production, collision and complete game ticks remain incomplete.\n";
    } catch(const std::exception& error) { std::cerr<<error.what()<<'\n'; return 1; }
}
