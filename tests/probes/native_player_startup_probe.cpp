#include "player/player_startup.hpp"
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace {
float f(std::uint32_t w){float v;std::memcpy(&v,&w,4);return v;}
std::int32_t i(std::uint32_t w){std::int32_t v;std::memcpy(&v,&w,4);return v;}
void put(std::uint8_t* out,std::size_t at,std::uint32_t w){for(unsigned b=0;b<4;++b)out[at+b]=static_cast<std::uint8_t>(w>>(b*8));}
void put_float(std::uint8_t* out,std::size_t at,float v){std::uint32_t w;std::memcpy(&w,&v,4);put(out,at,w);}
void put_vector(std::uint8_t* out,std::size_t at,sadx::NinjaVector v){put_float(out,at,v.x);put_float(out,at+4,v.y);put_float(out,at+8,v.z);}
std::uint32_t address(const sadx::TaskWorkHandle& w){return w?0x20000000+w->data_offset():0;}
}
//33 input words;1376 bytes represent primary64,motion64,player512,object124,
//contact132,auxiliary480. Typed values overlay the explicit fixture snapshot;
// no runtime guest memory view/interpreter exists in the production owner.
extern "C" int sadx_native_player_startup(const std::uint8_t* parameters,std::size_t parameter_size,
    const std::uint8_t* events,std::size_t event_size,const std::uint32_t* w,std::size_t words,
    std::uint16_t cw,std::uint32_t* out,std::size_t out_count,std::uint8_t* snapshot,std::size_t snapshot_size){
    if(!parameters||parameter_size!=1056||!events||event_size!=512||!w||words!=33||!out||out_count!=24||!snapshot||snapshot_size!=1376)return 1;
    try{
        auto arena=std::make_shared<sadx::TaskWorkArena>(0x20000000);sadx::TaskScheduler scheduler;
        if(!scheduler.initialize_source_pool(arena,std::shared_ptr<void>{}))return 2;
        auto task=scheduler.create_element(3,1,nullptr).task;if(!task)return 2;
        const auto primary=task->primary_work,motion_work=task->motion_work;
        std::uint8_t byte=static_cast<std::uint8_t>(w[2]);if(!arena->write(primary,2,&byte,1))return 3;
        const std::uint16_t flags=static_cast<std::uint16_t>(w[11]);if(!arena->write(primary,4,&flags,2))return 3;
        byte=static_cast<std::uint8_t>(w[0]);if(!arena->write(primary,8,&byte,1))return 3;
        byte=static_cast<std::uint8_t>(w[1]);if(!arena->write(primary,9,&byte,1))return 3;
        if(!arena->write(primary,0x14,w+12,12)||!arena->write(primary,0x20,w+15,12)||!arena->write(primary,0x2c,w+18,12)||
           !arena->write(motion_work,4,w+21,12)||!arena->write(motion_work,0x10,w+24,12)||!arena->write(motion_work,0x20,w+27,4)||
           !arena->write(motion_work,0x34,w+28,12))return 3;
        if(w[10]){if(!sadx::bind_player_task_primary(scheduler,task))return 3;if(w[2]>255)task->primary_state->task_id_at_2.reset();}
        if(w[6]){auto object=arena->allocate(124).work;if(!object||!arena->fill(object,0,0,124))return 3;byte=9;if(!arena->write(object,0,&byte,1)||!arena->set_reference(primary,0x3c,object))return 3;}
        if(w[7]){const auto remaining=sadx::TaskWorkArena::usable_bytes-*arena->free_head();if(remaining<=w[7]+32||!arena->allocate(remaining-w[7]-32))return 3;}
        if(!w[8])task->primary_work.reset();if(!w[9])task->motion_work.reset();
        auto motion=std::make_shared<sadx::PlayerMotionState>();
        motion->motion.speed={f(w[21]),f(w[22]),f(w[23])};motion->motion.acceleration={f(w[24]),f(w[25]),f(w[26])};
        motion->target_yaw=i(w[27]);motion->source_field_34=f(w[28]);motion->source_field_38=f(w[29]);motion->friction_scale=f(w[30]);
        sadx::PlayerStartupParameterTable table;std::memcpy(table.data(),parameters,1056);
        if(w[31] && w[1]<8)put(table[w[1]].data(),0x70,w[32]);
        sadx::PlayerEventFlags event_flags;std::memcpy(event_flags.data(),events,512);
        const sadx::PlayerStartupRuntime runtime{w[5]?&table:nullptr,w[4]?&event_flags:nullptr,nullptr};
        sadx::PlayerStartupRegistry registry;sadx::X87Context fp(cw);
        const auto result=sadx::initialize_player_startup(i(w[0]),task,w[3]?motion:nullptr,scheduler,registry,runtime,fp);
        const auto& record=*result.partial;
        std::memset(snapshot,0xcd,1376);std::memset(out,0,24*4);
        auto copy=[&](const sadx::TaskWorkHandle& work,std::size_t at,std::size_t count){if(work)std::memcpy(snapshot+at,arena->source_bytes().data()+work->data_offset(),count);};
        copy(primary,0,64);copy(motion_work,64,64);copy(record.player_work,128,512);
        auto object=arena->reference(primary,0x3c);if(object)copy(object.work,640,124);
        copy(record.contact_work,764,132);copy(record.auxiliary_work,896,480);
        if(task->primary_state){const auto& s=*task->primary_state;if(s.task_id_at_2)snapshot[2]=*s.task_id_at_2;
            if(s.action_at_0)snapshot[0]=*s.action_at_0;
            snapshot[4]=static_cast<std::uint8_t>(s.orientation.flags);snapshot[5]=static_cast<std::uint8_t>(s.orientation.flags>>8);
            snapshot[8]=s.player_index;snapshot[9]=s.character_id;for(unsigned k=0;k<3;++k)put(snapshot,0x14+k*4,static_cast<std::uint32_t>(s.orientation.angles[k]));
            if(s.position)put_vector(snapshot,0x20,*s.position);if(s.scale_at_2c)put_vector(snapshot,0x2c,*s.scale_at_2c);
        }
        if(record.motion){put_vector(snapshot,68,record.motion->motion.speed);put_vector(snapshot,80,record.motion->motion.acceleration);
            put(snapshot,96,static_cast<std::uint32_t>(record.motion->target_yaw));put_float(snapshot,116,record.motion->source_field_34);put_float(snapshot,120,record.motion->source_field_38);put_float(snapshot,124,record.motion->friction_scale);}
        if(record.player){const auto& p=*record.player;if(p.equipment_at_4){snapshot[132]=static_cast<std::uint8_t>(*p.equipment_at_4);snapshot[133]=static_cast<std::uint8_t>(*p.equipment_at_4>>8);}
            if(p.island_number_at_12){snapshot[146]=static_cast<std::uint8_t>(*p.island_number_at_12);snapshot[147]=static_cast<std::uint8_t>(*p.island_number_at_12>>8);}
            put_vector(snapshot,128+0x2c,p.motion.acceleration);put_vector(snapshot,128+0x38,p.motion.speed);
            if(p.shadow_scale_at_1fc)put_float(snapshot,128+0x1fc,*p.shadow_scale_at_1fc);
        }
        out[0]=static_cast<std::uint32_t>(result.failure);out[1]=result.source_boundary;out[2]=result.callee_boundary;out[3]=static_cast<std::uint32_t>(result.work_failure);out[4]=static_cast<std::uint32_t>(record.stage);out[5]=record.parameters.has_value();
        if(w[0]<8){out[6]=(registry.tasks[w[0]]?1:0)|(registry.task_work[w[0]]?2:0)|(registry.motion_work[w[0]]?4:0)|(registry.player_work[w[0]]?8:0);}
        out[7]=fp.exception_flags();out[8]=fp.control_word();out[9]=address(record.player_work);out[10]=address(record.contact_work);out[11]=address(record.auxiliary_work);
        const auto link=arena->reference(motion_work,0);if(link)out[12]=address(link.work);if(object)out[13]=address(object.work);
        out[14]=0x20000020;out[15]=static_cast<std::uint32_t>(scheduler.source_free_slots());out[16]=arena->free_head()?0x20000000+*arena->free_head():0;
        out[17]=task->primary_state && task->primary_state->task_id_at_2.has_value();out[18]=record.auxiliary_task?1:0;out[19]=task->primary_state?1:0;
        if(task->primary_state && task->primary_state->action_at_0)out[20]=1U|(std::uint32_t(*task->primary_state->action_at_0)<<8);
        if(record.player){const auto& p=*record.player;
            out[21]=(p.flags_at_10?1U:0U)|(p.no_control_time_at_a?2U:0U)|(p.moving_task_at_6c?4U:0U)|(p.cached_land_object_at_74?8U:0U);
            if(p.flags_at_10)out[22]=*p.flags_at_10;if(p.no_control_time_at_a)out[22]|=std::uint32_t(static_cast<std::uint16_t>(*p.no_control_time_at_a))<<16;
            out[23]=(p.moving_task_at_6c && !*p.moving_task_at_6c?1U:0U)|(p.cached_land_object_at_74 && !p.cached_land_object_at_74->id?2U:0U);
        }
        return 0;
    }catch(const sadx::X87Error&){return 4;}
}
