#include "player/player_scene_position.hpp"
#include <cstring>
namespace {
float number(std::uint32_t w){float v;std::memcpy(&v,&w,4);return v;}
std::int32_t integer(std::uint32_t w){std::int32_t v;std::memcpy(&v,&w,4);return v;}
std::uint32_t bits(float v){std::uint32_t w;std::memcpy(&w,&v,4);return w;}
sadx::NinjaVector vector(const std::uint32_t* w){return {number(w[0]),number(w[1]),number(w[2])};}
}
extern "C" int sadx_native_player_scene_helpers(std::uint32_t mode,const std::uint32_t* w,
    const std::uint8_t* source_constants,const std::uint8_t* parameters,
    const std::uint8_t* table,const std::uint8_t* unit,std::uint16_t cw,std::uint32_t* out){
    if(!w||!source_constants||!parameters||!table||!unit||!out)return -1;
    try{
        const auto c=sadx::decode_scene_position_constants(source_constants,124);const auto p=sadx::decode_player_parameters(parameters,132);if(!c||!p)return -2;
        sadx::PlayerTaskState task;task.position=vector(w);task.orientation={static_cast<std::uint16_t>(w[6]),{{integer(w[7]),integer(w[8]),integer(w[9])}}};task.player_index=static_cast<std::uint8_t>(w[13]);
        sadx::PlayerPhysicsState player;player.motion.speed=vector(w+15);player.flags_at_10=static_cast<std::uint16_t>(w[14]);
        sadx::PlayerMotionState world{};world.motion.speed=vector(w+3);
        sadx::PlayerContactWork contact;contact.direction_at_1c=vector(w+26);contact.vector_at_3c=vector(w+29);contact.vector_at_48=vector(w+32);contact.angle_x_at_34=integer(w[10]);contact.angle_z_at_38=integer(w[11]);contact.center_at_4=vector(w+35);
        sadx::NinjaMath::SineTable sine;std::memcpy(sine.data(),table,4100);sadx::NinjaMatrix identity;std::memcpy(identity.data(),unit,64);sadx::NinjaMatrixStack stack(std::vector<sadx::NinjaMatrix>(4,identity),1,0);sadx::PlayerCoordinates coordinates(sine,identity,stack);sadx::X87Context fp(cw);
        sadx::PlayerScenePositionResult result;
        if(mode==0)result=sadx::nudge_scene_normal(task,&world,contact,*c,fp);
        else if(mode==1){player.motion.speed.x=number(w[12]);result=sadx::rotate_scene_floor(task,player,*p.parameters,contact,*c,vector(w+38),coordinates,fp);}
        else {
            sadx::TaskScheduler scheduler;const auto moving=scheduler.append_task(0,sadx::TaskScheduler::source_no_op);if(!moving)return -3;player.moving_task_at_6c=moving.task;
            std::unordered_map<const sadx::Task*,sadx::ScenePlatformWork*> work_views;sadx::ScenePlatformWork force;
            if(mode==3){sadx::TaskWorkArena arena;const auto work=arena.allocate(56);if(!work)return -4;moving.task->force_work=work.work;
                for(auto& row:force.rows)row=sadx::ScenePlatformRow{sadx::ScenePlatformCallback{nullptr},std::array<std::int32_t,3>{{integer(w[23]),integer(w[24]),integer(w[25])}},vector(w+20)};
                work_views[moving.task.get()]=&force;result=sadx::move_scene_platform(task,player,contact,vector(w+17),{nullptr,&work_views},{c->single(0x7dcb2c),c->single(0x7dcb10)},fp);
            }else result=sadx::move_scene_platform(task,player,contact,vector(w+17),{nullptr,nullptr},{c->single(0x7dcb2c),c->single(0x7dcb10)},fp);
        }
        out[0]=static_cast<std::uint32_t>(result.failure);out[1]=result.source_boundary;if(!result)return 1;
        out[2]=bits(task.position->x);out[3]=bits(task.position->y);out[4]=bits(task.position->z);
        for(std::size_t i=0;i<3;++i)out[5+i]=static_cast<std::uint32_t>(task.orientation.angles[i]);
        out[8]=bits(world.motion.speed.x);out[9]=bits(world.motion.speed.y);out[10]=bits(world.motion.speed.z);
        out[11]=bits(contact.center_at_4->x);out[12]=bits(contact.center_at_4->y);out[13]=bits(contact.center_at_4->z);
        out[14]=fp.control_word();out[15]=fp.exception_flags();out[16]=stack.depth();for(std::size_t i=0;i<4;++i)for(std::size_t j=0;j<16;++j)out[17+i*16+j]=bits(stack.slots()[i][j]);return 0;
    }catch(const sadx::X87Error&){return -5;}catch(const sadx::CoordinateError&){return -6;}
}
extern "C" const char* sadx_native_player_scene_helpers_compiler(){return "Clang " __clang_version__ "; ARM64 C++17 O2 source-native scene helpers; explicit Bochs P5/P6 FPATAN model";}
