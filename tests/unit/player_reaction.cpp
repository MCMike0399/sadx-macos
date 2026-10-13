#include "player/player_reaction.hpp"
#include <iostream>
#include <stdexcept>
#include <array>
#include <fstream>

namespace {void require(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}}
int main(int argc,char** argv){
    try{
        require(argc==2,"usage: player-reaction-test REACTION_CONSTANTS");
        std::array<std::uint8_t,40> bytes{};std::ifstream input(argv[1],std::ios::binary);
        input.read(reinterpret_cast<char*>(bytes.data()),bytes.size());
        require(input.gcount()==40 && input.peek()==std::char_traits<char>::eof(),"source reaction constant artifact size");
        const auto decoded=sadx::decode_player_reaction_constants(bytes.data(),bytes.size());
        require(decoded && decoded->plane.axis.zero==0 && decoded->plane.axis.one==1 &&
                decoded->plane.parallel_cosine==0.9750000238418579F && decoded->plane.distance_scale==1 &&
                decoded->angle.negative_one==-1,"source reaction constants retain original encodings");
        const auto& c=*decoded;
        sadx::X87Context fp(0x37f);sadx::PlayerTaskState task{};
        sadx::PlayerPhysicsState player{};sadx::PlayerPositionCollisionScene scene{};
        sadx::CrtAsinRuntime runtime{1,{}};
        require(static_cast<bool>(sadx::react_player_to_contact(task,nullptr,player,scene,c,runtime,{},fp)),
                "ungrounded source path does not require motion/collision resources");
        task.orientation.flags=1;
        const auto missing=sadx::react_player_to_contact(task,nullptr,player,scene,c,runtime,{},fp);
        require(missing.failure==sadx::PlayerReactionFailure::null_motion && missing.source_boundary==0x43c59a,
                "grounded source motion dereference remains an explicit boundary");
        sadx::PlayerMotionState world{};world.motion.speed={0.5F,0,0};
        require(static_cast<bool>(sadx::react_player_to_contact(task,&world,player,scene,c,runtime,{},fp)),
                "source speed threshold returns before collision dereference");
        world.motion.speed={2,0,0};
        require(sadx::react_player_to_contact(task,&world,player,scene,c,runtime,{},fp).failure==
                sadx::PlayerReactionFailure::unknown_collision_reference,"unknown collision is not observed null");
        world.motion.speed={1,0,0};
        require(sadx::react_player_to_contact(task,&world,player,scene,c,runtime,{},fp).failure==
                sadx::PlayerReactionFailure::unknown_collision_reference,"exact one continues through source parity branch");
        world.motion.speed={2,0,0};
        scene.task_collision=sadx::PositionResourceRef{1};scene.works[1].flags=0x14;
        unsigned calls=0;
        const auto normal=[&](sadx::PlayerTaskState&,sadx::PlayerPositionCollisionScene&,sadx::X87Context&){
            ++calls;return sadx::PositionNormalResult{sadx::PlayerPositionFailure::none,sadx::NinjaVector{0,0,-1},0};
        };
        const auto no_floor=sadx::react_player_to_contact(task,&world,player,scene,c,runtime,normal,fp);
        require(no_floor.failure==sadx::PlayerReactionFailure::unknown_floor_normal && calls==1 &&
                player.source_vector_at_44 && player.source_vector_at_44->z==-1 && world.motion.speed.x==2,
                "raw normal is committed before missing floor boundary; speed not invented");
        player.floor_normal=sadx::NinjaVector{0,1,0};task.orientation.angles[1]=0;world.target_yaw=0;
        const auto done=sadx::react_player_to_contact(task,&world,player,scene,c,runtime,normal,fp);
        require(done && done.intersection && done.angle && world.motion.speed.x==2 &&
                world.motion.speed.y==0 && world.motion.speed.z==0 && world.target_yaw==task.orientation.angles[1],
                "source native plane/angle path preserves speed along intersection and updates shared yaw");
        player.floor_normal.reset();
        const auto bound=sadx::bind_player_contact_reaction(c,runtime,normal);
        const auto partial=bound(task,&world,player,scene,fp);
        require(partial.failure==sadx::PlayerPositionFailure::reaction_owner_failed &&
                partial.reaction_diagnostics && partial.reaction_diagnostics->failure==sadx::PlayerReactionFailure::unknown_floor_normal,
                "position binding retains the real lower failure facts");
        player.floor_normal=sadx::NinjaVector{0,1,0};world.motion.speed={2,0,1};
        runtime.matherr_control.reset();
        const auto changed=bound(task,&world,player,scene,fp);
        require(changed.reaction_diagnostics && changed.reaction_diagnostics->failure==sadx::PlayerReactionFailure::angle_failed &&
                changed.reaction_diagnostics->angle && changed.reaction_diagnostics->angle->crt_failure==sadx::CrtAsinFailure::unknown_matherr_policy,
                "binding observes changed live CRT policy rather than a stale snapshot");
        std::cout<<"Contact reaction: source gates, committed normal, missing resources and native plane/angle composition passed.\n";
        return 0;
    }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
