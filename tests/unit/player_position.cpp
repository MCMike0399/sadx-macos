#include "player/player_position.hpp"
#include "player/player_reaction.hpp"
#include <array>
#include <cstring>
#include <fstream>
#include <iostream>
#include <stdexcept>
using namespace sadx;
namespace {
void check(bool condition,const char* message){if(!condition)throw std::runtime_error(message);}
template<typename T>T read(const char* path){T value{};std::ifstream stream(path,std::ios::binary);stream.read(reinterpret_cast<char*>(&value),sizeof(value));check(stream.gcount()==sizeof(value)&&stream.peek()==std::char_traits<char>::eof(),"source artifact size");return value;}
PlayerTaskState task(std::uint16_t flags=0){return {{flags,{0,0,0}},0,0,{},NinjaVector{10,20,30}};}
PlayerPhysicsState player(float speed=3){PlayerPhysicsState p{{{11,12,13},{speed,0,0}},1,{21,22,23},{},0x12345678,0,{}};return p;}
PlayerMotionState world(){return {{{3,0,0},{31,32,33}},1234,41,42,.5F};}
PlayerPositionCollisionScene neutral(){PlayerPositionCollisionScene s;s.task_collision=PositionResourceRef{0};s.works[0].flags=0;return s;}
PlayerPositionCollisionScene wall(){auto s=neutral();auto& w=s.works[0];w.flags=0x14;w.my_info_index=0;w.info=PositionResourceRef{0};w.hit_work=PositionResourceRef{1};s.works[1].info=PositionResourceRef{1};s.info_tables[0].records[0].form=0;s.info_tables[0].records[0].damage=12;s.info_tables[0].records[0].a=1;s.info_tables[1].records[0].push=7;s.info_tables[1].records[0].attributes=0;s.info_tables[1].records[0].damage=0;s.info_tables[1].records[0].a=3;return s;}
}
int main(int argc,char** argv){try{
    check(argc==5,"usage: player-position-test SINE_TABLE UNIT ROTATION_CONSTANTS POSITION_CONSTANTS");
    const auto table=read<NinjaMath::SineTable>(argv[1]);const auto unit=read<NinjaMatrix>(argv[2]);
    const auto rbytes=read<std::array<std::uint8_t,88>>(argv[3]);PlayerRotationConstants rc;
    std::memcpy(rc.float_bits.data(),rbytes.data(),64);std::memcpy(rc.double_bits.data(),rbytes.data()+64,24);
    const auto bytes=read<std::array<std::uint8_t,32>>(argv[4]);const auto decoded=decode_player_position_constants(bytes.data(),bytes.size());check(bool(decoded),"source position constants");const auto& c=*decoded;
    check(c.speed_cap==15&&c.opposing_dot<-.97F,"pinned source cap/opposition identity");
    PlayerParameters params{};params.run_speed=5;params.crash_speed=10;
    PlayerPositionRuntime runtime;runtime.stage=1;runtime.act=0;runtime.crt_math.matherr_control=1;
    for(std::uint16_t pc:{0,0x200,0x300})for(std::uint16_t rounding:{0,0x400,0x800,0xc00}){
        X87Context fp(static_cast<std::uint16_t>(pc|rounding|0x7f));NinjaMatrixStack stack({unit,unit,unit},1,0);PlayerCoordinates coordinates(table,unit,stack);
        auto t=task();auto p=player();auto m=world();auto scene=neutral();PlayerPositionCallees callees;
        auto result=set_player_position(t,&m,p,params,scene,c,rc,runtime,callees,&coordinates,fp);
        check(!result&&result.failure==PlayerPositionFailure::missing_collision_owner&&result.source_boundary==0x44d4b1&&result.callee_address==0x449500,
              "position cannot silently skip required scene update");
        check(p.source_vector_at_44&&p.source_vector_at_44->x==0&&p.prior_attributes==0x12345678&&p.attributes==0,
              "source clear/prior-attribute writes precede missing collision owner");
        scene.task_collision.reset();p=player();result=set_player_position(t,&m,p,params,scene,c,rc,runtime,callees,&coordinates,fp);
        check(result.failure==PlayerPositionFailure::unavailable_reference&&p.prior_attributes==0x12345678,"unknown collision differs from a neutral/contact-free scene");
        scene=neutral();scene.task_collision=PositionResourceRef{};result=set_player_position(t,&m,p,params,scene,c,rc,runtime,callees,&coordinates,fp);
        check(result.failure==PlayerPositionFailure::null_required_reference,"source initial null collision pointer is invalid rather than empty contacts");
        check(position_collision_form(scene,0,true).outcome==-1,"small checked helper's null cwp policy differs from PSetPosition dereference");
        scene=neutral();check(position_collision_form(scene,-1,true).outcome==-1&&position_collision_form(scene,16,true).outcome==-1,
                              "original CCL index bounds precede resources");
        check(position_collision_form(scene,0,true).failure==PlayerPositionFailure::unavailable_reference,"checked form lookup still requires real info resource");
        scene=wall();p=player(20);t=task();m=world();
        result=set_player_position(t,&m,p,params,scene,c,rc,runtime,callees,&coordinates,fp);
        check(result.failure==PlayerPositionFailure::missing_normal_owner&&result.source_boundary==0x44ce8a,"wall path requires owning normal query");
        callees.normal=[](auto&,auto&,auto&){return PositionNormalResult{PlayerPositionFailure::none,NinjaVector{-1,0,0},0};};
        bool before_scene=false;
        callees.ordinary_collision=[&](auto&,auto* mw,auto& pw,auto&,auto&,auto*,auto&){before_scene=pw.motion.speed.x==0&&mw->motion.speed.x==0;pw.attributes=0x55;return PositionCollisionResult{PlayerPositionFailure::none,9,0};};
        result=set_player_position(t,&m,p,params,scene,c,rc,runtime,callees,&coordinates,fp);
        check(result&&result.outcome==2&&before_scene&&p.attributes==0x55&&p.motion.acceleration.x==11&&m.motion.acceleration.x==31,
              "speed cap clears speeds and retains early outcome before mandatory scene effects");
        scene=wall();p=player(3);m=world();callees.ordinary_collision=[](auto&,auto*,auto&,auto&,auto&,auto*,auto&){return PositionCollisionResult{PlayerPositionFailure::none,9,0};};
        result=set_player_position(t,&m,p,params,scene,c,rc,runtime,callees,&coordinates,fp);
        check(result.failure==PlayerPositionFailure::missing_hold_owner&&result.provisional_outcome==1,"low-speed opposite wall needs hold-object flags");
        callees.hold=[](auto&,auto&,auto&,auto&){return PositionHoldResult{PlayerPositionFailure::none,0x400,0};};
        result=set_player_position(t,&m,p,params,scene,c,rc,runtime,callees,&coordinates,fp);
        check(result&&result.outcome==1&&m.motion.speed.x==3&&m.motion.speed.z==0,"holding response uses source negated wall-normal components");
        // Known selected contacts remain intact; unknown fields stay unknown.
        check(!scene.info_tables[1].records[0].form&&scene.works[0].hit_work->id==1,"local position owner must not erase/invent collision records");
        callees.hold=[](auto&,auto&,auto&,auto&){return PositionHoldResult{PlayerPositionFailure::none,0,0};};
        scene.info_tables[1].records[0].attributes=0x2000;
        t=task(1);p=player(3);m=world();
        const PlayerReactionConstants reaction_constants{{{0,1},0.9750000238418579F,1},
            {-1,c.angle_units,c.positive_factor}};
        callees.reaction=bind_player_contact_reaction(reaction_constants,runtime.crt_math,callees.normal);
        result=set_player_position(t,&m,p,params,scene,c,rc,runtime,callees,&coordinates,fp);
        check(result.failure==PlayerPositionFailure::reaction_owner_failed && result.reaction_diagnostics &&
              result.reaction_diagnostics->failure==PlayerReactionFailure::unknown_floor_normal &&
              p.source_vector_at_44 && p.source_vector_at_44->x==-1,
              "real reaction binding retains committed normal and nested missing-floor facts through parent failure");
        scene.info_tables[1].records[0].attributes=0;
        t=task();
        p=player(3);m=world();m.motion.speed={1,0,0}; // Exact reciprocal at every selected PC/RC.
        result=set_player_position(t,&m,p,params,scene,c,rc,runtime,callees,&coordinates,fp);
        check(result&&m.motion.speed.x==0&&m.motion.speed.z==0,"zero sum response clears horizontal speeds without reaction fallback");
        p=player(3);m=world();callees.normal=[](auto&,auto&,auto&){return PositionNormalResult{PlayerPositionFailure::none,NinjaVector{-.5F,0,.866F},0};};
        result=set_player_position(t,&m,p,params,scene,c,rc,runtime,callees,&coordinates,fp);
        check(result&&m.motion.speed.x>1.4F&&m.motion.speed.x<1.6F&&m.motion.speed.z>2.5F,
              "source stack offset includes outstanding pushed argument when loading normalized X");
        scene=neutral();t=task(2);p=player();m=world();
        callees.ordinary_collision=[](auto& tw,auto*,auto&,auto&,auto&,auto*,auto&){tw.orientation.flags&=static_cast<std::uint16_t>(~2U);return PositionCollisionResult{PlayerPositionFailure::none,7,0};};
        result=set_player_position(t,&m,p,params,scene,c,rc,runtime,callees,&coordinates,fp);
        check(result&&result.outcome==7&&t.position->y==19.5F&&m.motion.speed.y==0&&p.floor_normal&&p.floor_normal->y==1&&stack.depth()==1,
              "pre-collision flag2 chooses tail; post-collision flag1 and matrix floor/position stores are live");
        scene=neutral();t=task(2);t.position.reset();p=player();m=world();
        result=set_player_position(t,&m,p,params,scene,c,rc,runtime,callees,&coordinates,fp);
        check(result.failure==PlayerPositionFailure::unavailable_task_position&&p.floor_normal&&stack.depth()==1,"unknown task position retains earlier floor/matrix writes");
        t=task(2);p=player();m=world();NinjaMatrixStack unknown({unit,unit,unit},1,{});PlayerCoordinates unknown_coords(table,unit,unknown);
        result=set_player_position(t,&m,p,params,scene,c,rc,runtime,callees,&unknown_coords,fp);
        check(result.failure==PlayerPositionFailure::coordinate_failed&&unknown.depth()==2&&!p.floor_normal,
              "failed calculate-vector does not fabricate normal or pop source frame");
        runtime.stage=9;runtime.act=2;t=task();p=player();m=world();bool alternate=false;
        callees.alternate_collision=[&](auto&,auto*,auto&,auto&,auto&,auto*,auto&){alternate=true;return PositionCollisionResult{PlayerPositionFailure::none,8,0};};
        result=set_player_position(t,&m,p,params,scene,c,rc,runtime,callees,&coordinates,fp);check(result&&alternate&&result.outcome==8,"source signed packed902 selects alternate collision owner");
        runtime.stage=1;runtime.act=0;
    }
    std::cout<<"player position local control and mandatory scene boundaries passed\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
