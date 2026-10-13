#include "player/player_rotation.hpp"
#include <cmath>
#include <cstring>
#include <fstream>
#include <iostream>
#include <stdexcept>

namespace {
void require(bool value,const char* message){if(!value)throw std::runtime_error(message);}
template<class T>T read(const char* path){T value{};std::ifstream file(path,std::ios::binary);
    file.read(reinterpret_cast<char*>(&value),sizeof(value));
    require(file.gcount()==static_cast<std::streamsize>(sizeof(value))&&file.peek()==std::char_traits<char>::eof(),"source artifact length mismatch");return value;}
sadx::PlayerPhysicsState player(float x=3){return {{{11,12,13},{x,1,-2}},1,{21,22,23},{},0,0,{}};}
sadx::PlayerTaskState task(std::uint16_t flags=0){return {{flags,{0,0,0}},0,0,{}, {}};}
sadx::PlayerMotionState world(){return {{{2,1,3},{31,32,33}},12345,41,42,0.5F};}
}
int main(int argc,char** argv){try{
    require(argc==4,"usage: player-rotation-test SOURCE_TABLE SOURCE_UNIT SOURCE_ROTATION_CONSTANTS");
    const auto table=read<sadx::NinjaMath::SineTable>(argv[1]);const auto unit=read<sadx::NinjaMatrix>(argv[2]);
    const auto bytes=read<std::array<std::uint8_t,88>>(argv[3]);std::uint64_t hash=0xcbf29ce484222325ULL;
    for(auto b:bytes)hash=(hash^b)*0x100000001b3ULL;
    require(hash==0xe32a778b8a4695e2ULL,"requires exact source rotation constants");
    sadx::PlayerRotationConstants c;std::memcpy(c.float_bits.data(),bytes.data(),64);std::memcpy(c.double_bits.data(),bytes.data()+64,24);
    for(std::uint16_t pc:{0,0x200,0x300})for(std::uint16_t rc:{0,0x400,0x800,0xc00}){
        sadx::X87Context fp(static_cast<std::uint16_t>(pc|rc|0x7f));
        sadx::NinjaMatrixStack stack({unit,unit,unit},1,0);sadx::PlayerCoordinates coordinates(table,unit,stack);
        sadx::PlayerRotationRuntime runtime;runtime.passed_player_no_control_timer=0;runtime.world_gravity=sadx::NinjaVector{0,-1,0};
        runtime.input.stage=1;runtime.input.no_control_timer[0]=0;runtime.input.normalized[0]=sadx::NormalizedPlayerInput{{},1};
        sadx::PlayerParameters parameters{};parameters.jog_speed=2;parameters.dash_speed=1;
        for(const auto test:std::array<std::array<std::int32_t,2>,5>{{{{4096,512}},{{4097,1024}},{{8192,2048}},{{8193,2048}},{{32768,63488}}}}){
            auto t=task();auto p=player();auto m=world();
            require(static_cast<bool>(sadx::rotate_player_regular(t,m,p,test[0],c,runtime,coordinates,fp))&&
                t.orientation.angles[1]==test[1],"regular source distance thresholds and half-turn tie");
            require(p.motion.acceleration.x==11&&m.motion.acceleration.x==31&&m.target_yaw==12345&&
                p.effect.x==21&&stack.depth()==1,"yaw helper preserves unrelated owned fields and matrix depth");
        }
        auto t=task();auto p=player(65);
        require(static_cast<bool>(sadx::rotate_player_blended(t,p,parameters,16384,c,coordinates,fp))&&
            t.orientation.angles[1]==65280,"blended source permits negative step; never clamps to a guessed yaw range");
        t=task(1);p=player();auto m=world();sadx::PlayerRotationRuntime unknown_input=runtime;unknown_input.input.stage.reset();
        const auto failed=sadx::rotate_player_regular(t,m,p,16384,c,unknown_input,coordinates,fp);
        require(!failed&&failed.failure==sadx::PlayerRotationFailure::input_unavailable&&failed.source_boundary==0x443d29&&
            failed.temporary_vector&&t.orientation.angles[1]==2048&&p.motion.speed.x==3&&stack.depth()==1,
            "input boundary failure preserves prior yaw/real transforms and leaves speed uncommitted");
        t=task();p=player();m=world();sadx::PlayerRotationRuntime unavailable=runtime;unavailable.passed_player_no_control_timer.reset();
        require(sadx::rotate_player_passive(t,m,p,parameters,c,unavailable,coordinates,fp).failure==
            sadx::PlayerRotationFailure::unavailable_timer,"passed-player timer is required independently from global input lookup");
        unavailable.passed_player_no_control_timer=1;unavailable.world_gravity.reset();
        require(static_cast<bool>(sadx::rotate_player_passive(t,m,p,parameters,c,unavailable,coordinates,fp))&&
            m.target_yaw==12345,"timer gate returns before unavailable gravity");
        unavailable.passed_player_no_control_timer=0;p.attributes=0x4000;
        require(static_cast<bool>(sadx::rotate_player_passive(t,m,p,parameters,c,unavailable,coordinates,fp)),
            "passive attribute gate precedes gravity owner");
        require(sadx::rotate_player_attribute(t,m,p,parameters,c,unavailable,coordinates,fp).failure==
            sadx::PlayerRotationFailure::unavailable_gravity,"special rotation has no passive 4000 attribute bypass");
        sadx::NinjaVector zero{0,-0.0F,0};(void)sadx::normalize_vector_extended(zero,{c.single(0x7dcb2c),c.single(0x7dcb10)},fp);
        require(zero.x==0&&zero.y==0&&!std::signbit(zero.y),"source normalization equality stores positive zero");
        sadx::NinjaVector nan{NAN,0,0};(void)sadx::normalize_vector_extended(nan,{c.single(0x7dcb2c),c.single(0x7dcb10)},fp);
        require(std::isnan(nan.x)&&std::isnan(nan.y)&&std::isnan(nan.z),"unordered normalize continues reciprocal, not zero fallback");
        sadx::NinjaVector xyz{3,4,0};const auto length=sadx::normalize_vector_extended(xyz,{c.single(0x7dcb2c),c.single(0x7dcb10)},fp);
        require(fp.to_float(length)==5&&xyz.x>0&&xyz.y>0&&xyz.z==0,"real scalar/normalize bodies produce source length and component stores");
        sadx::NinjaMatrixStack unknown_stack({unit,unit,unit},1,{});sadx::PlayerCoordinates unknown_coordinates(table,unit,unknown_stack);
        t=task();p=player();const auto partial=sadx::rotate_player_blended(t,p,parameters,100,c,unknown_coordinates,fp);
        require(!partial&&partial.source_boundary==0x443e83&&partial.nested_source_boundary==0x784db9&&partial.temporary_vector&&
            t.orientation.angles[1]==0&&p.motion.speed.x==3&&unknown_stack.depth()==2,
            "first coordinate failure preserves actual partial matrix ownership");
    }
    std::cout<<"Source rotation: four real owners, timer/input boundaries, yaw policies, source normalization and partial effects passed.\n"
             <<"Hardware FPATAN/runtime state and direction-reset resource queries remain unverified.\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
