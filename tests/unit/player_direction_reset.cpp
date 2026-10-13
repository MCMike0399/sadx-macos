#include "player/player_direction_reset.hpp"
#include "player/player_acceleration.hpp"
#include "player/player_acceleration_ground.hpp"
#include <fstream>
#include <iostream>
#include <stdexcept>
namespace {void require(bool b,const char* s){if(!b)throw std::runtime_error(s);}template<class T>T read(const char* p){T v{};std::ifstream f(p,std::ios::binary);f.read(reinterpret_cast<char*>(&v),sizeof(v));require(f.gcount()==static_cast<std::streamsize>(sizeof(v))&&f.peek()==std::char_traits<char>::eof(),"source artifact size mismatch");return v;}}
int main(int argc,char** argv){try{
    require(argc==5,"usage: direction-reset-test SOURCE_TABLE SOURCE_UNIT SOURCE_RESET_CONSTANTS SOURCE_GROUND_CONSTANTS");
    const auto table=read<sadx::NinjaMath::SineTable>(argv[1]);const auto unit=read<sadx::NinjaMatrix>(argv[2]);const auto raw=read<std::array<std::uint8_t,120>>(argv[3]);
    const auto c=sadx::decode_player_direction_reset_constants(raw.data(),raw.size());require(c.has_value(),"reset constants invalid");
    const auto ground_bits=read<std::array<std::uint32_t,24>>(argv[4]);sadx::GroundAccelerationConstants ground_constants{ground_bits};
    sadx::NinjaMatrixStack stack({unit,unit},1,0);sadx::PlayerCoordinates coordinates(table,unit,stack);
    auto path=std::make_shared<sadx::PlayerPathResource>(sadx::PlayerPathResource{0,3,12,{{0,0,4,{0,0,0}},{0,0,4,{4,0,0}},{0,0,4,{8,0,0}},{0,0,4,{12,0,0}}}});
    for(std::uint16_t pc:{0,0x200,0x300})for(std::uint16_t rc:{0,0x400,0x800,0xc00}){
        sadx::X87Context fp(static_cast<std::uint16_t>(pc|rc|0x7f));sadx::SseContext sse(0x1f80);
        sadx::PlayerTaskState task{{0x2000,{0,0,0}},0,0,{},sadx::NinjaVector{0,0,0},path};
        sadx::PlayerMotionState world{{{1,0,0},{0,0,0}},0,0,0,1};sadx::PlayerPhysicsState player{};
        player.motion.speed={2,0,0};player.path_distance=0;player.contact_work=sadx::PlayerContactWork{sadx::NinjaVector{0,1,0}};
        sadx::PlayerParameters parameters{};parameters.dash_speed=1;
        sadx::PlayerDirectionResetRuntime runtime;runtime.floor.cpu_sse2=1;runtime.floor.sse=&sse;runtime.inverse.matherr_control=1;
        const auto done=sadx::reset_player_direction(task,world,player,parameters,*c,runtime,coordinates,fp);
        require(done&&done.query&&done.query->source_return==true&&done.angle&&player.path_distance==2&&task.orientation.angles[1]==0&&world.target_yaw==0&&stack.depth()==1,
            "reset executes real path/plane/normalize/angle owners and leaves matrix stack unchanged");
        player.motion.speed.x=0.5F;player.path_distance=0;runtime.input.stage=1;runtime.input.no_control_timer[0]=0;
        runtime.input.normalized[0]=sadx::NormalizedPlayerInput{{},1};
        const auto clamped=sadx::reset_player_direction(task,world,player,parameters,*c,runtime,coordinates,fp);
        require(clamped&&player.motion.speed.x==1&&player.path_distance==1,
            "actual GetAnalog active result causes raw dash-speed store before path-distance update");
        player.motion.speed.x=0.5F;player.path_distance=0;runtime.input.stage.reset();
        const auto input_missing=sadx::reset_player_direction(task,world,player,parameters,*c,runtime,coordinates,fp);
        require(!input_missing&&input_missing.failure==sadx::PlayerDirectionResetFailure::input_unavailable&&
            input_missing.source_boundary==0x43c14e&&player.motion.speed.x==0.5F&&player.path_distance==0&&!input_missing.query,
            "input failure stops before either source player store");
        player.motion.speed.x=2;
        player.path_distance=-3;task.orientation.flags=0x2000;
        const auto outside=sadx::reset_player_direction(task,world,player,parameters,*c,runtime,coordinates,fp);
        require(outside&&outside.query&&outside.query->source_return==false&&player.path_distance==-1&&(task.orientation.flags&0x2000)==0,
            "actual query-false clears path flag after committing distance");
        player.path_distance=0;task.path_resource.reset();task.orientation.flags=0x2000;
        const auto missing=sadx::reset_player_direction(task,world,player,parameters,*c,runtime,coordinates,fp);
        require(!missing&&missing.failure==sadx::PlayerDirectionResetFailure::unavailable_path_resource&&player.path_distance==2&&!missing.path_status&&task.orientation.flags==0x2000,
            "unknown path resource fails after original player distance store but before query scratch store");
        player.path_distance=0;task.path_resource=std::shared_ptr<const sadx::PlayerPathResource>{};
        const auto null=sadx::reset_player_direction(task,world,player,parameters,*c,runtime,coordinates,fp);
        require(!null&&null.failure==sadx::PlayerDirectionResetFailure::null_path_resource&&null.path_status&&null.path_status->distance==2,
            "observed null resource remains distinct and preserves argument FSTP before source null read");
        const auto binding=sadx::bind_ground_direction_reset(*c,runtime,coordinates);
        // Mutate borrowed runtime after binding and use a different live player
        // parameter object; a copied runtime/parameter snapshot would fail.
        runtime.input.stage=1;runtime.input.normalized[0]=sadx::NormalizedPlayerInput{0,1};
        sadx::PlayerAccelerationRuntime ground_runtime;ground_runtime.world_gravity=sadx::NinjaVector{0,0,0};ground_runtime.input=runtime.input;
        sadx::GroundAccelerationCallees callees;callees.direction_reset=binding;
        callees.world_to_player=[&](const auto& orientation,auto& value,auto& context){return coordinates.world_to_player(orientation,value,context);};
        task.path_resource.reset();task.orientation.flags=0x2000;player.path_distance=0;player.dotp=1;
        auto live_parameters=parameters;live_parameters.dash_speed=3;
        const auto ground=sadx::get_player_acceleration_ground(task,world,player,live_parameters,ground_constants,ground_runtime,callees,fp);
        require(!ground&&ground.source_boundary==0x44c691&&ground.callee_address==0x43c130&&
            ground.direction_reset&&ground.direction_reset->failure==sadx::PlayerDirectionResetFailure::unavailable_path_resource&&
            player.motion.speed.x==3&&player.path_distance==3,
            "real ground binding consumes live runtime/parameters and propagates full reset partial facts");
    }
    std::cout<<"Direction reset: real path/plane owners, distance stores, source-false flag mutation and unknown/null boundaries passed.\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
