#include "player/player_contact_response.hpp"
#include <cassert>
#include <cstring>
#include <fstream>
#include <iostream>
#include <vector>
namespace {
std::vector<std::uint8_t> read(const char* path){std::ifstream f(path,std::ios::binary);return {std::istreambuf_iterator<char>(f),{}};}
}
int main(int argc,char** argv){using namespace sadx;
    if(argc!=3)return 1;const auto raw=read(argv[1]),sine_bytes=read(argv[2]);
    const auto constants=decode_contact_response_constants(raw.data(),raw.size());assert(constants&&sine_bytes.size()==4100);
    NinjaMath::SineTable sine;std::memcpy(sine.data(),sine_bytes.data(),4100);
    const std::optional<NinjaVector> gravity=NinjaVector{0,-1,0};CrtAsinRuntime inverse{1,{}};
    PlayerContactResponseRuntime runtime{inverse,gravity,&sine,{}};
    PlayerContactWork contact;PlayerMeshQueryState state;
    PlayerTaskState task{{0,{}},0,0,{}};PlayerMotionState world{};PlayerPhysicsState player{};
    PlayerParameters parameters{};parameters.run_speed=2;parameters.jog_speed=1;parameters.crash_speed=4;
    PlayerContactResponseActors actors{&task,&world,&player,&parameters,0};
    auto seed=[&](){contact={};contact.contact_count_at_28=1;contact.center_at_4=NinjaVector{0,0,0};
        contact.direction_at_1c=NinjaVector{0,1,0};contact.displacement_at_10=NinjaVector{-4,0,0};
        contact.vector_at_60=NinjaVector{0,0,0};contact.vector_at_6c=NinjaVector{-3,1,2};contact.vector_at_78=NinjaVector{.5F,0,-.25F};
        MeshContactPayload p{};p.normal=p.oriented_normal={1,0,0};p.point_at_20=p.point_at_2c={1,0,0};state.responses.fill(p);
        world.motion.speed={-5,0,0};world.target_yaw=0x5000;task.orientation.angles[1]=0x1000;
        player.prior_attributes=0;player.source_vector_at_44=NinjaVector{0,1,0};player.motion.speed={3,0,0};};
    for(unsigned pc:{0U,0x200U,0x300U})for(unsigned rc:{0U,0x400U,0x800U,0xc00U}){
        const auto cw=static_cast<std::uint16_t>(pc|rc|0x7f);X87Context fp(cw);
        for(unsigned kind=0;kind<5;++kind){
            seed();auto r=respond_player_contacts(static_cast<PlayerContactResponseEntry>(kind),contact,state,actors,*constants,runtime,fp);
            assert(r&&r.outcome && r.distances[0]==1);
            if(kind<2){assert(*r.outcome==1 && contact.displacement_at_10->x==-1);}
            if(kind==2){assert(*r.outcome==0 && contact.displacement_at_10->x==-4);}
            if(kind==4){assert(*r.outcome==1 && world.motion.speed.x<0 && world.motion.speed.x>-.21F);}
            assert(fp.control_word()==cw);
        }
        seed();player.motion.speed.x=5;
        auto r=respond_player_contacts(PlayerContactResponseEntry::at_43d360,contact,state,actors,*constants,runtime,fp);
        assert(r&&r.outcome==2 && contact.vector_at_6c->x==0 && world.motion.speed.x==-5);
    }
    X87Context fp(0x37f);seed();
    contact.contact_count_at_28.reset();auto r=respond_player_contacts(PlayerContactResponseEntry::at_43cf90,contact,state,actors,*constants,runtime,fp);
    assert(!r && r.source_boundary==0x43cf99 && !r.selection_scratch_initialized);
    contact={};contact.contact_count_at_28=0;PlayerContactResponseActors unavailable{nullptr,nullptr,nullptr,nullptr,{}};
    const std::optional<NinjaVector> absent_gravity;PlayerContactResponseRuntime absent{inverse,absent_gravity,nullptr,{}};
    for(unsigned kind=0;kind<5;++kind)assert(respond_player_contacts(static_cast<PlayerContactResponseEntry>(kind),contact,state,unavailable,*constants,absent,fp).outcome==0);
    seed();contact.center_at_4.reset();r=respond_player_contacts(PlayerContactResponseEntry::at_43cf90,contact,state,actors,*constants,runtime,fp);
    assert(!r && r.source_boundary==0x43cfc4);
    seed();contact.direction_at_1c.reset();r=respond_player_contacts(PlayerContactResponseEntry::at_43cf90,contact,state,actors,*constants,runtime,fp);
    assert(!r && r.source_boundary==0x43d008 && r.distances[0]==1 && r.classifications[0]==0);
    seed();r=respond_player_contacts(PlayerContactResponseEntry::at_43cf90,contact,state,actors,*constants,absent,fp);
    assert(!r && r.failure==PlayerContactResponseFailure::unavailable_gravity && r.source_boundary==0x43d026);
    seed();actors.attributes.reset();r=respond_player_contacts(PlayerContactResponseEntry::at_43cce0,contact,state,actors,*constants,runtime,fp);
    assert(!r && r.source_boundary==0x43ce31 && r.candidates==1 && r.selected==0);actors.attributes=0;
    seed();PlayerContactResponseRuntime no_sine{inverse,gravity,nullptr,{}};
    r=respond_player_contacts(PlayerContactResponseEntry::at_43cce0,contact,state,actors,*constants,no_sine,fp);
    assert(!r && r.failure==PlayerContactResponseFailure::missing_sine_table && r.source_boundary==0x43ce47);
    seed();inverse.matherr_control.reset();r=respond_player_contacts(PlayerContactResponseEntry::at_43cf90,contact,state,actors,*constants,runtime,fp);
    assert(!r && r.failure==PlayerContactResponseFailure::projection_failed && r.projection && r.nested_source_boundary);
    inverse.matherr_control=1;fp.set_control_word(0x37f);
    seed();state.responses[0]->point_at_2c={2000,0,0};
    r=respond_player_contacts(PlayerContactResponseEntry::at_43cf90,contact,state,actors,*constants,runtime,fp);
    assert(!r && r.failure==PlayerContactResponseFailure::unavailable_stack_word && r.source_boundary==0x43d108);
    seed();contact.contact_count_at_28=16;for(auto& p:state.responses)p->point_at_2c={2000,0,0};
    r=respond_player_contacts(PlayerContactResponseEntry::at_43cf90,contact,state,actors,*constants,runtime,fp);
    assert(r && r.selected==-1 && r.outcome==1 && contact.displacement_at_10->x<0 && contact.displacement_at_10->x>-1e-40F);
    seed();contact.contact_count_at_28=17;
    r=respond_player_contacts(PlayerContactResponseEntry::at_43cf90,contact,state,actors,*constants,runtime,fp);
    assert(!r && r.failure==PlayerContactResponseFailure::unavailable_response && r.source_boundary==0x43cfc0 && r.distances[15]);
    seed();r=respond_player_contacts(static_cast<PlayerContactResponseEntry>(99),contact,state,actors,*constants,runtime,fp);
    assert(!r && r.failure==PlayerContactResponseFailure::invalid_entry);
    std::cout<<"Contact response bodies passed12 CW profiles, source selection, alias scratch and required-boundary tests\n";
}
