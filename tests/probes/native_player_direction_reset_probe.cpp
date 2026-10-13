#include "path_probe_data.hpp"
#include "player/player_direction_reset.hpp"
#include "player/player_acceleration.hpp"
#include <cstddef>
#include <cstring>
extern "C" int sadx_native_player_direction_reset(const std::uint8_t* table,std::size_t table_size,
    const std::uint8_t* unit,std::size_t unit_size,const std::uint8_t* constants,std::size_t constants_size,
    const std::uint32_t* w,std::size_t count,std::uint16_t cw,std::uint32_t* out,std::size_t out_count){
    if(!table||table_size!=4100||!unit||unit_size!=64||!constants||constants_size!=120||!w||count<36||
       w[27]>(count-36)/5||count!=36+w[27]*5||!out||out_count!=50)return 1;
    try{
        const auto c=sadx::decode_player_direction_reset_constants(constants,120);if(!c)return 2;
        sadx::NinjaMath::SineTable sine;std::memcpy(sine.data(),table,4100);sadx::NinjaMatrix identity;std::memcpy(identity.data(),unit,64);
        sadx::NinjaMatrixStack stack({identity,identity},1,0);sadx::PlayerCoordinates coordinates(sine,identity,stack);
        sadx::PlayerTaskState task{{static_cast<std::uint16_t>(w[0]),{sadx_probe::integer(w[1]),sadx_probe::integer(w[2]),sadx_probe::integer(w[3])}},0,0,{},{},{}};
        if(w[4])task.position=sadx_probe::vector(w+5);
        if(w[8])task.path_resource=std::shared_ptr<const sadx::PlayerPathResource>{};
        if(w[8]==2)task.path_resource=std::make_shared<sadx::PlayerPathResource>(sadx_probe::resource(w[25],w[26],sadx_probe::number(w[28]),w+36,w[27]));
        sadx::PlayerPhysicsState player{};player.motion.speed=sadx_probe::vector(w+11);if(w[9])player.path_distance=sadx_probe::number(w[10]);
        if(w[19]){player.contact_work=sadx::PlayerContactWork{};if(w[20])player.contact_work->direction_at_1c=sadx_probe::vector(w+21);}
        sadx::PlayerMotionState world{{sadx_probe::vector(w+14),{0,0,0}},sadx_probe::integer(w[17]),0,0,1};sadx::PlayerParameters p{};p.dash_speed=sadx_probe::number(w[18]);
        sadx::X87Context fp(cw);sadx::SseContext sse(w[31]);sadx::PlayerDirectionResetRuntime runtime;
        runtime.input.stage=1;runtime.input.no_control_timer[0]=0;runtime.input.normalized[0]=sadx::NormalizedPlayerInput{{},sadx_probe::number(w[24])};
        if(w[29])runtime.floor.cpu_sse2=w[30];runtime.floor.sse=&sse;if(w[32])runtime.floor.fallback_control_word=static_cast<std::uint16_t>(w[33]);if(w[34])runtime.inverse.matherr_control=w[35];
        const auto result=sadx::reset_player_direction(task,world,player,p,*c,runtime,coordinates,fp);std::memset(out,0,50*4);
        out[0]=static_cast<std::uint32_t>(result.failure);out[1]=result.source_boundary;out[2]=result.nested_source_boundary;out[3]=task.orientation.flags;
        out[4]=static_cast<std::uint32_t>(task.orientation.angles[1]);out[5]=static_cast<std::uint32_t>(world.target_yaw);out[6]=sadx_probe::bits(player.motion.speed.x);
        out[7]=player.path_distance.has_value();if(player.path_distance)out[8]=sadx_probe::bits(*player.path_distance);out[9]=fp.control_word();out[10]=fp.exception_flags();out[11]=sse.mxcsr();
        if(result.path_status)out[12]=sadx_probe::store_status(*result.path_status,out+13);
        out[33]=result.projected_displacement.has_value();if(result.projected_displacement){out[34]=sadx_probe::bits(result.projected_displacement->x);out[35]=sadx_probe::bits(result.projected_displacement->y);out[36]=sadx_probe::bits(result.projected_displacement->z);}
        out[37]=result.projected_velocity.has_value();if(result.projected_velocity){out[38]=sadx_probe::bits(result.projected_velocity->x);out[39]=sadx_probe::bits(result.projected_velocity->y);out[40]=sadx_probe::bits(result.projected_velocity->z);}
        if(result.query){out[41]=static_cast<std::uint32_t>(result.query->failure);out[42]=result.query->source_return.has_value();out[43]=result.query->source_return.value_or(false);out[44]=result.query->source_boundary;}
        if(result.angle){out[45]=static_cast<std::uint32_t>(result.angle->failure);out[46]=static_cast<std::uint32_t>(result.angle->crt_failure);out[47]=result.angle->source_boundary;out[48]=result.angle->nested_source_boundary;}
        out[49]=stack.depth();return 0;
    }catch(const sadx::X87Error&){return 3;}catch(const sadx::SseError&){return 4;}catch(const sadx::CoordinateError&){return 5;}
}
