#include "player/player_contact_response.hpp"
#include <cstring>
namespace {
sadx::NinjaMath::SineTable table;
std::optional<sadx::PlayerContactResponseConstants> constants;
float number(std::uint32_t u){float f;std::memcpy(&f,&u,4);return f;}
std::uint32_t bits(float f){std::uint32_t u;std::memcpy(&u,&f,4);return u;}
std::int32_t signed_word(std::uint32_t u){std::int32_t x;std::memcpy(&x,&u,4);return x;}
sadx::NinjaVector vec(const std::uint32_t* w){return {number(w[0]),number(w[1]),number(w[2])};}
}
extern "C" int sadx_native_contact_response_init(const float* sine,const std::uint8_t* raw,std::size_t size){
    if(!sine)return 1;constants=sadx::decode_contact_response_constants(raw,size);
    if(!constants)return 1;std::memcpy(table.data(),sine,table.size()*4);return 0;
}
extern "C" int sadx_native_contact_response(unsigned entry,std::uint16_t cw,
        const std::uint32_t* w,const std::uint32_t* responses,std::uint32_t* out){
    if(!constants||!w||!responses||!out)return 1;std::memset(out,0,64*4);
    try{
        sadx::PlayerContactWork contact;contact.contact_count_at_28=static_cast<std::uint16_t>(w[0]);
        contact.center_at_4=vec(w+22);contact.direction_at_1c=vec(w+25);contact.displacement_at_10=vec(w+28);
        contact.vector_at_60=vec(w+31);contact.vector_at_6c=vec(w+34);contact.vector_at_78=vec(w+37);
        sadx::PlayerTaskState task{{0,{}},0,0,{}};task.orientation.angles[1]=signed_word(w[5]);
        sadx::PlayerMotionState world{};world.target_yaw=signed_word(w[6]);world.motion.speed=vec(w+16);
        sadx::PlayerPhysicsState player{};player.prior_attributes=w[4];player.motion.speed=vec(w+13);player.source_vector_at_44=vec(w+19);
        sadx::PlayerParameters parameters{};parameters.run_speed=number(w[7]);parameters.jog_speed=number(w[8]);parameters.crash_speed=number(w[9]);
        const std::optional<sadx::NinjaVector> gravity=vec(w+10);
        sadx::CrtAsinRuntime inverse;if(w[2])inverse.matherr_control=w[3];
        const sadx::PlayerContactResponseRuntime runtime{inverse,gravity,&table,{}};
        const sadx::PlayerContactResponseActors actors{&task,&world,&player,&parameters,w[1]};
        sadx::PlayerMeshQueryState state;
        for(std::size_t i=0;i<w[0]&&i<16;++i){sadx::MeshContactPayload payload;std::memcpy(&payload,responses+i*22,88);state.responses[i]=payload;}
        sadx::X87Context fp(cw);
        const auto r=sadx::respond_player_contacts(static_cast<sadx::PlayerContactResponseEntry>(entry),contact,state,actors,*constants,runtime,fp);
        out[0]=static_cast<std::uint32_t>(r.failure);out[1]=r.source_boundary;out[2]=r.nested_source_boundary;
        out[3]=r.outcome.has_value();if(r.outcome)out[4]=static_cast<std::uint32_t>(*r.outcome);
        out[5]=fp.control_word();out[6]=fp.exception_flags();
        auto store=[&](unsigned at,const sadx::NinjaVector& v){out[at]=bits(v.x);out[at+1]=bits(v.y);out[at+2]=bits(v.z);};
        store(7,*contact.displacement_at_10);store(10,*contact.vector_at_6c);store(13,world.motion.speed);store(16,*player.source_vector_at_44);
        out[19]=static_cast<std::uint32_t>(task.orientation.angles[1]);out[20]=static_cast<std::uint32_t>(world.target_yaw);
        out[21]=static_cast<std::uint32_t>(r.candidates);out[22]=static_cast<std::uint32_t>(r.selected);out[23]=bits(r.closest);
        for(unsigned i=0;i<16;++i){if(r.distances[i]){out[24]|=1U<<i;out[26+i]=bits(*r.distances[i]);}
            if(r.classifications[i]){out[25]|=1U<<i;out[42+i]=*r.classifications[i];}}
        if(r.normal_sum){store(58,*r.normal_sum);out[61]=1;}
        out[62]=r.selection_scratch_initialized;return 0;
    }catch(const sadx::X87Error&){return 2;}
}
